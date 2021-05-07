/*******************************************************************************************************************//**
 *  \file contact-tracing.c
 *  \brief This file is composed by 3 processes:
 *
 *  Broadcast sender process;
 *
 *  Broadcast Receiver process;
 *
 *  MQTT Client process.
 *
 *  The first one sends its client id (generated in the MQTT Client process) at regular intervals via SIMPLE UDP
 *  library to the other motes of the same network.
 *
 *  The receiver listens to incoming broadcast messages and, upon receiving, it inserts the data into a queue buffer
 *  which it is later published on the MQTT connection topic (the whole operation uses mutexes for concurrency
 *  purposes).
 *
 *  The last process is the first one to be fired and calls the other ones.
 *  First it establishes the connection with the broker, then it waits for a PROCESS_EVENT_TIMER (etimer o ctimer).
 *  When registering the MQTT connection, it enables a mqtt_event callback function that handles most of the MQTT events.
 *  \author Sonny
 *  \version 1.0
 **********************************************************************************************************************/
 #include "contiki.h"
#include "dev/button-hal.h"
#include "lib/random.h"
#include "lib/sensors.h"
#include "net/ipv6/sicslowpan.h"
#include "net/ipv6/uip-debug.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/uip.h"
#include "net/routing/routing.h"
#include "os/net/app-layer/mqtt/mqtt.h"
#include "os/sys/log.h"
#include "os/sys/mutex.h"
#include "simple-udp.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "sys/etimer.h"
#include "mqtt-client.h"
#include <string.h>
#include <stdio.h>

//LOG_LEVEL prints LOG messages, currently on INFO mode
#define LOG_MODULE "mqtt-client"
#define LOG_LEVEL LOG_LEVEL_INFO

//MQTT client configs
#define MQTT_CLIENT_ORG_ID "polimi"
#define MQTT_CLIENT_TYPE_ID "mqtt-client"
#define MQTT_CLIENT_USERNAME "mqtt-client-username"
#define MQTT_CLIENT_AUTH_TOKEN "AUTHTOKEN"

//MQTT broker configs
#define MQTT_BROKER_IP "fd00::1"
#define MQTT_BROKER_PORT 1883

//MQTT topics
#define MQTT_INFECTION_TOPIC "kafka-to-mqtt-alerts"
#define MQTT_CONNECTION_TOPIC "motes-connections"
#define MQTT_ALERT_TOPIC "motes-alerts"

//Buffer sizes
#define MAX_TCP_SEGMENT_SIZE 32
#define BUFFER_SIZE 64
#define APP_BUFFER_SIZE 512
#define ECHO_REQ_PAYLOAD_LEN 20
#define MAX_QUEUE_SIZE 100
#define CLIENT_ID_SIZE 34
#define FIRST_JSON_INDEX 14
#define SECOND_JSON_INDEX 65

//Intervals
#define  PERIODIC_PUBLISH_INTERVAL 60*CLOCK_SECOND
#define BROADCAST_INTERVAL 35*CLOCK_SECOND
#define MQTT_KEEP_ALIVE_INTERVAL 30
#define CLOCK_MINUTE 60*CLOCK_SECOND

//UDP Ports
#define SENDER_UDP_PORT 8765
#define RECEIVER_UDP_PORT 5678

/*******************************************************************************************************************//**
 * \brief MQTT enums based on MQTT_EVENTS.
 *
 * - SUBSCRIBE -> MQTT_EVENT_SUBSCRIBE;
 *
 * - FIRE_PUBLISH, CONTINUE_PUBLISH -> MQTT_EVENT_PUBLISH (connection);
 *
 * - END_PUBLISH -> MQTT_EVENT_PUBLISH ended;
 *
 * - FIRE_ALERT -> MQTT_EVENT_PUBLISH (alert)
 *
 * - NA -> NOT ASSIGNED
 **********************************************************************************************************************/
enum mqtt_action {
    SUBSCRIBE, FIRE_PUBLISH, CONTINUE_PUBLISH, END_PUBLISH, FIRE_ALERT, NA
};

//Connections
static struct simple_udp_connection broadcast_connection, broadcast_receiver_connection;
static struct mqtt_connection conn;

//MQTT settings
static char client_id[CLIENT_ID_SIZE];
static char pub_topic[BUFFER_SIZE];
static char sub_topic[BUFFER_SIZE];
static char app_buffer[APP_BUFFER_SIZE];

//Support structs
static struct mqtt_message *msg_ptr = 0;
static struct uip_icmp6_echo_reply_notification echo_reply_notification;

//MQTT ctimers used for publishing connections and alerts.
static struct ctimer mqtt_callback_timer;
static struct ctimer mqtt_alert_timer;

//MQTT etimers
static struct etimer mqtt_connect_timer;
static struct etimer ping_parent_timer;
static struct etimer mqtt_register_timer;

//Simple UDP timers
static struct etimer broadcast_timer;

//Queue buffer for MQTT
char *message_to_publish[MAX_QUEUE_SIZE][MAX_QUEUE_SIZE];
int messages_length = 0;

//Other stuff
static char *buf_ptr;
static int def_rt_rssi = 0;
static mutex_t mqtt_mutex;
static int buffer_index = 0;
static enum mqtt_action mqtt_action_ptr = NA;
static enum mqtt_action alert = FIRE_ALERT;


//CONTIKI-NG PROCESSES
PROCESS(mqtt_client_process, "MQTT Client Main Process");
PROCESS(broadcast, "Broadcast Process");
PROCESS(broadcast_receiver, "Broadcast Receiver Process");
AUTOSTART_PROCESSES(&mqtt_client_process, &broadcast, &broadcast_receiver);

/*******************************************************************************************************************//**
 * \brief Function used for displaying incoming messages from subscribed topic.
 *
 * @param topic MQTT subscribed topic.
 * @param topic_len MQTT subscribed topic length.
 * @param chunk Content of the received message.
 * @param chunk_len Content length.
 **********************************************************************************************************************/
static void pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk, uint16_t chunk_len) {
    char current_client_id_temp[CLIENT_ID_SIZE];
    char sender_client_id[CLIENT_ID_SIZE];
    int k =0;
    for(int i=FIRST_JSON_INDEX; i<FIRST_JSON_INDEX+strlen(client_id); i++) {
        current_client_id_temp[k] = (char)chunk[i];
        k++;
    }
    current_client_id_temp[k]='\0';
    if(strcmp(client_id, current_client_id_temp)==0) {
        k=0;
        for (int i = SECOND_JSON_INDEX; i < SECOND_JSON_INDEX + strlen(client_id); ++i) {
            sender_client_id[k] = (char) chunk[i];
            k++;
        }
        current_client_id_temp[k]='\0';
        LOG_INFO("Alert Received From: %s\n", sender_client_id);
        LOG_DBG("Payload=%s\n", chunk);
    }
}
/*******************************************************************************************************************//**
 * \brief Callback function for echo reply.
 *
 * @param source Source IP address.
 * @param ttl Time To Live.
 * @param data Payload data.
 * @param datalen Data length.
 **********************************************************************************************************************/
static void echo_reply_handler(uip_ipaddr_t *source, uint8_t ttl, uint8_t *data, uint16_t datalen) {
    if (uip_ip6addr_cmp(source, uip_ds6_defrt_choose())) {
        def_rt_rssi = sicslowpan_get_last_rssi();
    }
}

/*******************************************************************************************************************//**
 * \brief Function used for MQTT subscribing. Currently using MQTT_QOS_LEVEL_0 (No PUBACK).
 *
 * @param topic MQTT subscribe topic.
 **********************************************************************************************************************/
static void subscribe(char *topic) {
    mqtt_status_t status;
    LOG_DBG("Subscribing!\n");
    status = mqtt_subscribe(&conn, NULL, topic, MQTT_QOS_LEVEL_0);
    if (status == MQTT_STATUS_OUT_QUEUE_FULL) {
        LOG_ERR("Tried to subscribe but command queue was full!\n");
    }
}

/*******************************************************************************************************************//**
 * \brief Function for building the publish topic, it checks if the topic exceeds the BUFFER_SIZE.
 *
 *  @param topic MQTT topic.
 *  @return 1 if successful, 0 otherwise.
 **********************************************************************************************************************/
static int construct_pub_topic(char *topic) {
    int len = snprintf(pub_topic, BUFFER_SIZE, "%s", topic);
    if (len < 0 || len >= BUFFER_SIZE) {
        LOG_DBG("Pub Topic: %d, Buffer %d\n", len, BUFFER_SIZE);
        return 0;
    }
    return 1;
}

/*******************************************************************************************************************//**
 * \brief Function used to build json format messages for the mqtt_publish and fires it.
 *
 * It distinguishes between a CONNECTION or ALERT type and it creates different json strings.
 *
 * The publish function uses MQTT_QOS_LEVEL_1 because the PUBACK identifies the first publish of the queue buffer.
 *
 * After the first publish, all the following ones follow the case FIRE_PUBLISH covered by the mqtt_callback() function,
 * in order to reset the ctimer responsible for multiple publishes.
 *
 * @param message Client ID received by the broadcast_receiver process or the alert fired by the mqtt_alert_timer.
 * @param action Type of action: CONNECTION or ALERT.
 **********************************************************************************************************************/
void send_to_mqtt_broker(char *message, char *action) {
    int len;
    int remaining = APP_BUFFER_SIZE;
    char *event_type;
    buf_ptr = app_buffer;
    event_type = action;
    if (strcmp(action, "CONNECTION") == 0) {
        len = snprintf(buf_ptr, 500,
                       "{"
                       "\"EventType\":\"%s\","
                       "\"ClientId\":\"%s\","
                       "\"SenderClientId\":\"%s\"", event_type, client_id, message);
    } else if (strcmp(action, "ALERT") == 0) {
        len = snprintf(buf_ptr, remaining,
                       "{"
                       "\"EventType\":\"%s\","
                       "\"ClientId\":\"%s\"",
                       event_type, client_id);
    }
    if (len < 0 || len >= remaining) {
        LOG_WARN("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
        return;
    }
    remaining -= len;
    buf_ptr += len;
    len = snprintf(buf_ptr, remaining, "}");
    if (len < 0 || len >= remaining) {
        LOG_ERR("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
        return;
    }
    LOG_DBG("App Buffer: %s\n", app_buffer);
    mqtt_publish(&conn, NULL, pub_topic, (uint8_t *) app_buffer, strlen(app_buffer), MQTT_QOS_LEVEL_1, MQTT_RETAIN_OFF);
    LOG_INFO("Published!\n");
}

static void mqtt_callback(void *ptr);

/*******************************************************************************************************************//**
 * \brief Function for emptying the queue buffer, effectively assigning all action strings to NULL and freeing all
 * dynamic message strings.
 **********************************************************************************************************************/
static void clear_mqtt_buffer() {
    for (int i = 0; i < messages_length; ++i) {
        free(message_to_publish[i][0]);
        message_to_publish[i][1] = NULL;
    }
    messages_length = 0;
}

/*******************************************************************************************************************//**
 * \brief Function used to fire publish action by calling send_to_mqtt_broker().
 *
 * It is used only for CONNECTIONS (ALERTS are fired directly from send_to_mqtt_broker()), with the first if clause it
 * checks if the current publish queue is empty and if so it skips the action.
 *
 * After the send_to_mqtt_broker() it resets the publish c_timer for the mqtt_callback function.
 *
 * @param topic MQTT publish topic.
 **********************************************************************************************************************/
static void publish(char *topic) {
    construct_pub_topic(topic);
    LOG_DBG("First Publish\n");
    if (message_to_publish[buffer_index][0] == NULL || buffer_index >= messages_length) {
        LOG_DBG("Empty buffer, skipping publish\n");
        mqtt_action_ptr = END_PUBLISH;
        ctimer_set(&mqtt_callback_timer, 1 * CLOCK_SECOND, mqtt_callback, &mqtt_action_ptr);
        return;
    }
    send_to_mqtt_broker(message_to_publish[buffer_index][0], "CONNECTION");
    mqtt_action_ptr = CONTINUE_PUBLISH;
    buffer_index++;
    ctimer_set(&mqtt_callback_timer, 3 * CLOCK_SECOND, mqtt_callback, &mqtt_action_ptr);
}

/*******************************************************************************************************************//**
 * \brief Function used for main MQTT actions:
 *
 * - SUBSCRIBE: it checks if the client is connected to the RPL border router and subscribes to the
 * MQTT_INFECTION_TOPIC; otherwise it resets the subscribe action ctimer.
 *
 * - FIRE_PUBLISH: it fires the first publish on the MQTT_CONNECTION_TOPIC if the mutex is free, otherwise it resets
 * the publish action ctimer.
 *
 * - CONTINUE_PUBLISH: it fires each value in the queue buffer by calling itself, if the queue is empty it fires the
 * END_PUBLISH timer.
 *
 * - END_PUBLISH: it resets the publish timer (mqtt_callback_timer) within a PERIODIC_PUBLISH_INTERVAL and resets the
 * buffer_index (indicating the emptiness of the buffer).
 *
 * - FIRE_ALERT: it builds the alert publish topic and publish an alert on it, ultimately it resets the alert timer.
 *
 * @param ptr Address of enum mqtt_action (mqtt_action_ptr or alert) used for switch case in order to recognize the
 * action.
 **********************************************************************************************************************/

static void mqtt_callback(void *ptr) {
    switch ((*(enum mqtt_action *) ptr)) {
        case SUBSCRIBE:
            if (mqtt_ready(&conn) && conn.out_buffer_sent) {
                subscribe(MQTT_INFECTION_TOPIC);
            } else {
                ctimer_reset(&mqtt_callback_timer);
            }
            break;
        case FIRE_PUBLISH:
            if (mutex_try_lock(&mqtt_mutex)) {
                publish(MQTT_CONNECTION_TOPIC);
            } else {
                ctimer_reset(&mqtt_callback_timer);
            }
            break;
        case CONTINUE_PUBLISH:
            if (message_to_publish[buffer_index][0] == NULL || buffer_index >= messages_length) {
                LOG_DBG("Empty buffer, skipping publish\n");
                mqtt_action_ptr = END_PUBLISH;
                ctimer_set(&mqtt_callback_timer, 1 * CLOCK_SECOND, mqtt_callback, &mqtt_action_ptr);
                break;
            }
            send_to_mqtt_broker(message_to_publish[buffer_index][0], "CONNECTION");
            buffer_index++;
            ctimer_set(&mqtt_callback_timer, 3 * CLOCK_SECOND, mqtt_callback, &mqtt_action_ptr);
            break;
        case END_PUBLISH:
            clear_mqtt_buffer();
            mqtt_action_ptr = FIRE_PUBLISH;
            buffer_index = 0;
            ctimer_set(&mqtt_callback_timer, PERIODIC_PUBLISH_INTERVAL, mqtt_callback, &mqtt_action_ptr);
            mutex_unlock(&mqtt_mutex);
            break;
        case FIRE_ALERT:
            construct_pub_topic(MQTT_ALERT_TOPIC);
            send_to_mqtt_broker(client_id, "ALERT");
            int random = random_rand() % 5 + 1;
            LOG_INFO("Random interval for alert set: %d\n", random);
            ctimer_set(&mqtt_alert_timer, random*CLOCK_MINUTE, mqtt_callback, &alert);
            break;
        default:
            break;
    }
}

/*******************************************************************************************************************//**
  * \brief Function called by the mqtt_client_process when receiving packets from the broker, based on their type.
  *
  * - MQTT_EVENT_CONNECTED (MQTT client is connected to the broker): it fires the subscribe() function by setting the
  * ctimer of the mqtt_callback() function.
  *
  * - MQTT_EVENT_DISCONNECTED (MQTT client is disconnected from the broker): it uses the process_poll() function in
  * order to restart the MQTT connection.
  *
  * - MQTT_EVENT_PUBLISH (MQTT client received a publish on the subscribed topic): it prints alerts received from the
  * related topic.
  *
  * - MQTT_EVENT_SUBACK (MQTT client is subscribed to a topic): it fires the first publish (followed by the remaining
  * until the queue is fully processed) and it sets a random timer for the alert event.
  *
  * - MQTT_EVENT_UNSUBACK (MQTT client successfully unsubscribed from a topic).
  *
  * - MQTT_EVENT_PUBACK (MQTT client successfully published a message to a topic).
  *
  * @param m MQTT connection.
  * @param event PROCESS_EVENT of type MQTT, it is received for every action between the client and the broker.
  * @param data Content of the PROCESS_EVENT.
**********************************************************************************************************************/
static void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data) {

    switch (event) {
        case MQTT_EVENT_CONNECTED: {
            mqtt_action_ptr = SUBSCRIBE;
            ctimer_set(&mqtt_callback_timer, 1 * CLOCK_SECOND, mqtt_callback, &mqtt_action_ptr);
            LOG_INFO("Application has a MQTT connection\n");
            break;
        }
        case MQTT_EVENT_DISCONNECTED: {
            process_poll(&mqtt_client_process);
            LOG_INFO("MQTT Disconnect. Reason %u\n", *((mqtt_event_t *) data));
            break;
        }
        case MQTT_EVENT_PUBLISH: {
            msg_ptr = data;
            if (msg_ptr->first_chunk) {
                msg_ptr->first_chunk = 0;
                LOG_DBG("Application received publish for topic '%s'. Payload "
                        "size is %i bytes.\n", msg_ptr->topic, msg_ptr->payload_length);
            }
            pub_handler(msg_ptr->topic, strlen(msg_ptr->topic),
                        msg_ptr->payload_chunk, msg_ptr->payload_length);
            break;
        }
        case MQTT_EVENT_SUBACK: {
            mqtt_action_ptr = FIRE_PUBLISH;
            buffer_index = 0;
            ctimer_set(&mqtt_callback_timer, 1 * CLOCK_SECOND, mqtt_callback, &mqtt_action_ptr);
            int random = random_rand() % 5 + 1;
            LOG_INFO("Random interval for alert set: %d\n", random);
            ctimer_set(&mqtt_alert_timer, random*CLOCK_MINUTE, mqtt_callback, &alert);
            LOG_INFO("Application is subscribed to topic successfully\n");
            break;
        }
        case MQTT_EVENT_UNSUBACK: {
            LOG_DBG("Application is unsubscribed to topic successfully\n");
            break;
        }
        case MQTT_EVENT_PUBACK: {
            LOG_DBG("Publishing complete.\n");
            break;
        }
        default:
            LOG_DBG("Application got a unhandled MQTT event: %i\n", event);
            break;
    }
}

/*******************************************************************************************************************//**
 * \brief Function for building client ID and subscribe topic.
 **********************************************************************************************************************/
static void update_config(void) {
    snprintf(client_id, CLIENT_ID_SIZE, "d:%s:%s:%02x%02x%02x%02x%02x%02x", MQTT_CLIENT_ORG_ID, MQTT_CLIENT_TYPE_ID,
             linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1], linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
             linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
    snprintf(sub_topic, BUFFER_SIZE, "%s", "#");
}

/*******************************************************************************************************************//**
 * \brief Function for inserting client IDs (ALERTS or CONNECTIONS) into the queue and then published to the MQTT
 * broker.
 *
 * @param message Client ID received by the broadcast_receiver process or the alert fired by the mqtt_alert_timer.
 * @param action Type of action: CONNECTION or ALERT.
 **********************************************************************************************************************/
static void insert_mqtt_buffer(char *message, char *action) {
    if (mutex_try_lock(&mqtt_mutex)) {
        for (int i = 0; i < messages_length; ++i) {
            if (message_to_publish[i][0] != NULL && strcmp(message, message_to_publish[i][0]) == 0) {
                mutex_unlock(&mqtt_mutex);
                return;
            }
        }
        message_to_publish[messages_length][0] = strdup(message);
        message_to_publish[messages_length][1] = action;
        messages_length++;
        mutex_unlock(&mqtt_mutex);
        return;
    } else {
        return;
    }
}

/*******************************************************************************************************************//**
 * \brief Function checking if RPL connection with the border router has been established.
 * @return true is successful, false otherwise.
 **********************************************************************************************************************/
static bool have_connectivity(void) {
    if (uip_ds6_get_global(ADDR_PREFERRED) == NULL ||
        uip_ds6_defrt_choose() == NULL) {
        return false;
    }
    return true;
}

/*******************************************************************************************************************//**
 * \brief Callback function used when receiving an UDP packet on clients reached by the UDP packets sent by the
 * broadcast process (Unused in this implementation).
 *
 * @param c Broadcast connection (simple-udp.c).
 * @param sender_addr Sender IP address (IPv6).
 * @param sender_port Sender UDP port.
 * @param receiver_addr Receiver IP address (IPv6).
 * @param receiver_port Receiver UDP port.
 * @param data Data received in UDP payload.
 * @param datalen Data length.
 **********************************************************************************************************************/
static void broadcast_callback(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr, uint16_t sender_port,
                               const uip_ipaddr_t *receiver_addr, uint16_t receiver_port, const uint8_t *data,
                               uint16_t datalen) {
}

/*******************************************************************************************************************//**
 * \brief Callback function used when receiving an UDP packet on broadcast_receiver process.
 *
 * It inserts incoming client IDs into the queue buffer.
 *
 * @param c Broadcast connection (simple-udp.c).
 * @param sender_addr Sender IP address (IPv6).
 * @param sender_port Sender UDP port.
 * @param receiver_addr Receiver IP address (IPv6).
 * @param receiver_port Receiver UDP port.
 * @param data Data received in UDP payload.
 * @param datalen Data length.
 **********************************************************************************************************************/
static void broadcast_receiver_callback(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
                                        uint16_t sender_port, const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
                                        const uint8_t *data,
                                        uint16_t datalen) {
    insert_mqtt_buffer((char *) data, "CONNECTION");
    LOG_INFO("Received %s\n", data);
}

/*******************************************************************************************************************//**
 * \brief Contiki process thread responsible for sending broadcast UDP packets.
 *
 * @param data Content of PROCESS_EVENT.
 * @param ev PROCESS_EVENT passed to the PROCESS.
 * @param broadcast Process name.
 **********************************************************************************************************************/
PROCESS_THREAD(broadcast, ev, data) {
    uip_ipaddr_t addr;
    PROCESS_BEGIN();
                etimer_set(&broadcast_timer, BROADCAST_INTERVAL);
                simple_udp_register(&broadcast_connection, SENDER_UDP_PORT, NULL, RECEIVER_UDP_PORT,
                                    broadcast_callback);

                while (1) {
                    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&broadcast_timer));
                    LOG_DBG("Sending broadcast\n");
                    uip_create_linklocal_allnodes_mcast(&addr);
                    simple_udp_sendto(&broadcast_connection, client_id, strlen(client_id), &addr);
                    etimer_reset(&broadcast_timer);
                    LOG_INFO("Broadcast sent\n");
                }
    PROCESS_END();
}

/*******************************************************************************************************************//**
 * \brief Contiki process thread responsible for receiving broadcast UDP packets.
 *
 * @param data Content of PROCESS_EVENT.
 * @param ev PROCESS_EVENT passed to the PROCESS.
 * @param broadcast_receiver Process name.
 **********************************************************************************************************************/
PROCESS_THREAD(broadcast_receiver, ev, data) {
    PROCESS_BEGIN();
                simple_udp_register(&broadcast_receiver_connection, RECEIVER_UDP_PORT, NULL, SENDER_UDP_PORT,
                                    broadcast_receiver_callback);

    PROCESS_END();
}

/*******************************************************************************************************************//**
 * \brief Contiki process thread responsible for mqtt client's actions.
 *
 * @param data Content of PROCESS_EVENT.
 * @param ev PROCESS_EVENT passed to the PROCESS when calling process_poll() or using etimer_set(&example_etimer).
 * @param mqtt_client_process Process name.
 **********************************************************************************************************************/
PROCESS_THREAD(mqtt_client_process, ev, data) {
    PROCESS_BEGIN();
                LOG_INFO("MQTT Client Main Process\n");
                update_config();
                def_rt_rssi = 0x8000000;
                uip_icmp6_echo_reply_callback_add(&echo_reply_notification, echo_reply_handler);
                etimer_set(&mqtt_register_timer, 3 * CLOCK_SECOND);
                while (1) {
                    PROCESS_YIELD();
                    switch (ev) {
                        case PROCESS_EVENT_TIMER:
                            if (data == &mqtt_register_timer) {
                                mqtt_register(&conn, &mqtt_client_process, client_id, mqtt_event,
                                              MAX_TCP_SEGMENT_SIZE);
                                mqtt_set_username_password(&conn, MQTT_CLIENT_USERNAME, MQTT_CLIENT_AUTH_TOKEN);
                                conn.auto_reconnect = 0;
                                etimer_set(&ping_parent_timer, 3 * CLOCK_SECOND);
                            } else if (data == &ping_parent_timer) {
                                if (have_connectivity()) {
                                    uip_icmp6_send(uip_ds6_defrt_choose(), ICMP6_ECHO_REQUEST, 0,
                                                   ECHO_REQ_PAYLOAD_LEN);
                                    etimer_set(&mqtt_connect_timer, 2 * CLOCK_SECOND);
                                } else {
                                    etimer_set(&ping_parent_timer, 3 * CLOCK_SECOND);
                                    LOG_WARN("Connecting...\n");
                                }
                            } else if (data == &mqtt_connect_timer) {
                                if (have_connectivity()) {
                                    mqtt_connect(&conn, MQTT_BROKER_IP, MQTT_BROKER_PORT,
                                                 MQTT_KEEP_ALIVE_INTERVAL, MQTT_CLEAN_SESSION_ON);
                                } else {
                                    etimer_reset(&mqtt_connect_timer);
                                }
                            }
                            break;
                        case PROCESS_EVENT_POLL:
                            etimer_reset(&mqtt_connect_timer);
                            break;
                        default:
                            break;
                    }
                }
    PROCESS_END();
}