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

//LOG_LEVEL prints LOG messages, currently on DEBUG mode
#define LOG_MODULE "mqtt-client"
#define LOG_LEVEL LOG_LEVEL_DBG

//MQTT client configs
#define MQTT_CLIENT_ORG_ID "polimi"
#define MQTT_CLIENT_TYPE_ID "mqtt-client"
#define MQTT_CLIENT_USERNAME "mqtt-client-username"
#define MQTT_CLIENT_AUTH_TOKEN "AUTHTOKEN"

//MQTT broker configs
#define MQTT_BROKER_IP "fd00::1"
#define MQTT_BROKER_PORT 1883

//MQTT topics
#define MQTT_INFECTION_TOPIC "kafka-to-mqtt-alert"
#define MQTT_CONNECTION_TOPIC "motes-connections"

//Buffer sizes
#define MAX_TCP_SEGMENT_SIZE    32
#define BUFFER_SIZE 64
#define APP_BUFFER_SIZE 512
#define ECHO_REQ_PAYLOAD_LEN 20
#define MAX_QUEUE_SIZE 100

//Intervals
#define  PERIODIC_PUBLISH_INTERVAL 60*CLOCK_SECOND
#define BROADCAST_INTERVAL 35*CLOCK_SECOND
#define MQTT_KEEP_ALIVE_INTERVAL 30

//UDP Ports
#define SENDER_UDP_PORT 8765
#define RECEIVER_UDP_PORT 5678

enum action {
    CONNECTION, ALERT
};

enum mqtt_action {
    SUBSCRIBE, FIRE_PUBLISH, CONTINUE_PUBLISH, END_PUBLISH, NA
};

//Connections
static struct simple_udp_connection broadcast_connection, broadcast_receiver_connection;
static struct mqtt_connection conn;

//Setting strings
static char client_id[BUFFER_SIZE];
static char pub_topic[BUFFER_SIZE];
static char sub_topic[BUFFER_SIZE];
static char app_buffer[APP_BUFFER_SIZE];

//Support structs
static struct mqtt_message *msg_ptr = 0;
static struct uip_icmp6_echo_reply_notification echo_reply_notification;

static struct ctimer mqtt_callback_timer;
//MQTT timers
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
static uint16_t seq_nr_value = 0;
static int def_rt_rssi = 0;
static mutex_t mqtt_mutex;
static int buffer_index = 0;
static enum mqtt_action mqtt_action_ptr = NA;

//PROCESSES
PROCESS(mqtt_client_process, "MQTT Client Main Process");

PROCESS(broadcast, "Broadcast Process");

PROCESS(broadcast_receiver, "Broadcast Receiver Process");
AUTOSTART_PROCESSES(&mqtt_client_process, &broadcast, &broadcast_receiver);

//LOG for incoming publishes
static void pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk, uint16_t chunk_len) {
    LOG_DBG("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len, chunk_len);
}

//Callback function for echo reply
static void echo_reply_handler(uip_ipaddr_t *source, uint8_t ttl, uint8_t *data, uint16_t datalen) {
    if (uip_ip6addr_cmp(source, uip_ds6_defrt_choose())) {
        def_rt_rssi = sicslowpan_get_last_rssi();
    }
}

//Subscribe to MQTT topic
static void subscribe(char *topic) {
    mqtt_status_t status;
    LOG_DBG("Subscribing!\n");
    status = mqtt_subscribe(&conn, NULL, topic, MQTT_QOS_LEVEL_0);
    if (status == MQTT_STATUS_OUT_QUEUE_FULL) {
        LOG_ERR("Tried to subscribe but command queue was full!\n");
    }
}

//Build  publish topic
static int construct_pub_topic(char *topic) {
    int len = snprintf(pub_topic, BUFFER_SIZE, "%s", topic);
    if (len < 0 || len >= BUFFER_SIZE) {
        LOG_INFO("Pub Topic: %d, Buffer %d\n", len, BUFFER_SIZE);
        return 0;
    }
    return 1;
}

//MQTT publish
void send_to_mqtt_broker(char *message, char *action) {
    int len;
    int remaining = APP_BUFFER_SIZE;
    char *event_type;
    seq_nr_value++;
    buf_ptr = app_buffer;
    event_type = action;
    if (strcmp(action, "CONNECTION") == 0) {
        len = snprintf(buf_ptr, 500,
                       "{"
                       "\"EventType\":\"%s\","
                       "\"ClientId\":\"%s\","
                       "\"SenderClientId\":\"%s\","
                       "\"Seq\":%d", event_type, client_id, message,
                       seq_nr_value);
    } else if (strcmp(action, "ALERT") == 0) {
        len = snprintf(buf_ptr, remaining,
                       "{"
                       "\"EventType\":\"%s\","
                       "\"ClientId\":\"%s\","
                       "\"Seq\":%d", event_type, client_id,
                       seq_nr_value);
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
    LOG_DBG("Publish!\n");
}

static void mqtt_callback(void *ptr);

//Clear MQTT buffer by assigning each action to NULL and freeing messages
static void clear_mqtt_buffer() {
    for (int i = 0; i < messages_length; ++i) {
        free(message_to_publish[i][0]);
        //message_to_publish[i][0] = NULL;
        message_to_publish[i][1] = NULL;
    }
    messages_length = 0;
}


static void publish(char *topic) {
    if (construct_pub_topic(topic) == 0) {
        LOG_ERR("State Config Error");
    }
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

        default:
            break;
    }
}

//MQTT event
static void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data) {

    switch (event) {
        case MQTT_EVENT_CONNECTED: {
            mqtt_action_ptr = SUBSCRIBE;
            ctimer_set(&mqtt_callback_timer, 1 * CLOCK_SECOND, mqtt_callback, &mqtt_action_ptr);
            LOG_DBG("Application has a MQTT connection\n");
            break;
        }
        case MQTT_EVENT_DISCONNECTED: {
            process_poll(&mqtt_client_process);
            LOG_DBG("MQTT Disconnect. Reason %u\n", *((mqtt_event_t *) data));
            break;
        }
        case MQTT_EVENT_PUBLISH: {
            msg_ptr = data;
            /* Implement first_flag in publish message? */
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
            LOG_DBG("Application is subscribed to topic successfully\n");
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


//Update config
static void update_config(void) {
    snprintf(client_id, BUFFER_SIZE, "d:%s:%s:%02x%02x%02x%02x%02x%02x", MQTT_CLIENT_ORG_ID, MQTT_CLIENT_TYPE_ID,
             linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1], linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
             linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
    snprintf(sub_topic, BUFFER_SIZE, "%s", "#");
    seq_nr_value = 0;
}


//Insert into queue
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
        LOG_DBG("SKIPPO NON HO IL MUTEX\n");
        return;
    }
}


//Check net connectivity
static bool have_connectivity(void) {
    if (uip_ds6_get_global(ADDR_PREFERRED) == NULL ||
        uip_ds6_defrt_choose() == NULL) {
        return false;
    }
    return true;
}

//Simple UDP broadcast sender callback functions
static void broadcast_callback(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr, uint16_t sender_port,
                               const uip_ipaddr_t *receiver_addr, uint16_t receiver_port, const uint8_t *data,
                               uint16_t datalen) {
}

//Simple UDP broadcast
static void broadcast_receiver_callback(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
                                        uint16_t sender_port, const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
                                        const uint8_t *data,
                                        uint16_t datalen) {
    insert_mqtt_buffer((char *) data, "CONNECTION");
    LOG_DBG("Received %s\n", data);
}


//Broadcast PROCESS
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
                    LOG_DBG("Broadcast sent\n");
                }
    PROCESS_END();
}

//Broadcast receiver PROCESS
PROCESS_THREAD(broadcast_receiver, ev, data) {
    PROCESS_BEGIN();
                simple_udp_register(&broadcast_receiver_connection, RECEIVER_UDP_PORT, NULL, SENDER_UDP_PORT,
                                    broadcast_receiver_callback);

    PROCESS_END();
}

PROCESS_THREAD(mqtt_client_process, ev, data) {
    PROCESS_BEGIN();
                LOG_DBG("MQTT Client Main Process\n");
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