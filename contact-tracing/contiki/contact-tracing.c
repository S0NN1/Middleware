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
#include <strings.h>
#include <stdio.h>

#define LOG_MODULE "mqtt-client"
#define LOG_LEVEL LOG_LEVEL_DBG
#define MQTT_CLIENT_ORG_ID "polimi"
#define MQTT_CLIENT_TYPE_ID "mqtt-client"
#define MAX_TCP_SEGMENT_SIZE    32
#define BUFFER_SIZE 64
#define APP_BUFFER_SIZE 512
#define MQTT_BROKER_IP "fd00::1"
#define MQTT_BROKER_PORT 1883
#define ECHO_REQ_PAYLOAD_LEN 20
#define MQTT_CLIENT_USERNAME "mqtt-client-username"
#define MQTT_CLIENT_AUTH_TOKEN "AUTHTOKEN"
static struct simple_udp_connection broadcast_connection, broadcast_receiver_connection;
static char client_id[BUFFER_SIZE];
static char pub_topic[BUFFER_SIZE];
static char sub_topic[BUFFER_SIZE];
static struct mqtt_connection conn;
static char app_buffer[APP_BUFFER_SIZE];
enum action {
    CONNECTION, ALERT
};

static struct mqtt_message *msg_ptr = 0;
static struct etimer mqtt_connect_timer;
static struct etimer mqtt_connected_timer;
static struct etimer mqtt_publish_timer;
static struct etimer ping_parent_timer;
static struct etimer mqtt_register_timer;
static char *buf_ptr;
static uint16_t seq_nr_value = 0;
static struct uip_icmp6_echo_reply_notification echo_reply_notification;
static int def_rt_rssi = 0;
//static struct etimer alert_to_kafka_timer;
static struct etimer broadcast_timer;
char *message_to_publish[100][100];

int messages_length = 0;

#define MQTT_INFECTION_TOPIC "#"
mutex_t mqtt_mutex;


PROCESS(mqtt_client_main_process, "MQTT Client Main Process");

PROCESS(broadcast, "Broadcast Process");


PROCESS(broadcast_receiver, "Broadcast Receiver Process");

AUTOSTART_PROCESSES(&mqtt_client_main_process/*, &broadcast, &broadcast_receiver*/);


static void

pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk,

            uint16_t chunk_len) {

    LOG_DBG("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic,

            topic_len, chunk_len);

}

static void

echo_reply_handler(uip_ipaddr_t *source, uint8_t ttl, uint8_t *data,

                   uint16_t datalen) {

    if (uip_ip6addr_cmp(source, uip_ds6_defrt_choose())) {

        def_rt_rssi = sicslowpan_get_last_rssi();

    }

}


static void
mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data) {
    switch (event) {
        case MQTT_EVENT_CONNECTED: {
            process_poll(&mqtt_client_main_process);
            LOG_DBG("Application has a MQTT connection\n");
            break;
        }
        case MQTT_EVENT_DISCONNECTED: {
            //TODO Process poll
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
            LOG_DBG("Application is subscribed to topic successfully\n");
            break;
        }
        case MQTT_EVENT_UNSUBACK: {
            LOG_DBG("Application is unsubscribed to topic successfully\n");
            break;
        }
        case MQTT_EVENT_PUBACK: {
            etimer_set(&mqtt_publish_timer, 0);
            LOG_DBG("Publishing complete.\n");
            break;
        }
        default:
            LOG_DBG("Application got a unhandled MQTT event: %i\n", event);
            break;
    }
}


static int

construct_pub_topic(char *topic) {

    int len = snprintf(pub_topic, BUFFER_SIZE, "%s", topic);


    if (len < 0 || len >= BUFFER_SIZE) {

        LOG_INFO("Pub Topic: %d, Buffer %d\n", len, BUFFER_SIZE);

        return 0;

    }


    return 1;

}


static void
update_config(void) {
    //TODO BUFFER 33
    snprintf(client_id, BUFFER_SIZE, "d:%s:%s:%02x%02x%02x%02x%02x%02x",

             MQTT_CLIENT_ORG_ID, MQTT_CLIENT_TYPE_ID,

             linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],

             linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],

             linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
    //TODO BUFFER 19
    snprintf(sub_topic, BUFFER_SIZE, "%s", "#");
    /* Reset the counter */
    seq_nr_value = 0;
}


static void
subscribe(char *topic) {
    mqtt_status_t status;
    LOG_DBG("Subscribing!\n");
    status = mqtt_subscribe(&conn, NULL, topic, MQTT_QOS_LEVEL_0);
    if (status == MQTT_STATUS_OUT_QUEUE_FULL) {
        LOG_ERR("Tried to subscribe but command queue was full!\n");
    }
}

static void insert_mqtt_buffer(char *message, char *action) {
    while (true) {
        if (mutex_try_lock(&mqtt_mutex)) {
            printf("\n\nHo preso il lock per inserire\n\n");
            printf("\nStampo il buffer");
            for (int i = 0; i <= messages_length; ++i) {
                printf("\n%s", message_to_publish[i][0]);
            }
            for (int i = 0; i <= messages_length; ++i) {
                printf("\nConfronto %s con %s", message, message_to_publish[i][0]);
                if (message_to_publish[i][0] != NULL && strcmp(message, message_to_publish[i][0]) == 0) {
                    mutex_unlock(&mqtt_mutex);
                    printf("\nEsco\n");
                    return;
                }
            }
            message_to_publish[messages_length][0] = strdup(message);
            message_to_publish[messages_length][1] = action;
            messages_length++;
            mutex_unlock(&mqtt_mutex);
            printf("\nStampo il buffer");
            for (int i = 0; i <= messages_length; ++i) {
                printf("\n%s", message_to_publish[i][0]);
            }
            printf("\n\nHo rilasciato il lock per inserire\n\n");
            break;
        }
    }
}

static void clear_mqtt_buffer() {
    while (true) {
        if (mutex_try_lock(&mqtt_mutex)) {
            for (int i = 0; i < messages_length; ++i) {
                free(message_to_publish[i][0]);
                message_to_publish[i][1] = NULL;
                /*message_to_publish[i][0] = NULL;
                message_to_publish[i][1] = NULL;*/
            }
            messages_length = 0;
            mutex_unlock(&mqtt_mutex);
            break;
        }
    }

}

void send_to_mqtt_broker(char *message, char *action) {
    int len;
    int remaining = APP_BUFFER_SIZE;
    char *event_type;
    seq_nr_value++;
    buf_ptr = app_buffer;
    event_type = action;
    printf("\n\n\nMessage: %s", message);
    printf("\nAction: %s\n\n\n", action);
    if (strcmp(action, "CONNECTION") == 0) {
        len = snprintf(buf_ptr, 500,

                       "{"

                       "\"EventType\":\"%s\","

                       "\"ClientId\":\"%s\","

                       "\"SenderClientId\":\"%s\","

                       "\"Seq\":%d,", event_type, client_id, message,

                       seq_nr_value);

    } else if (strcmp(action, "ALERT") == 0) {
        len = snprintf(buf_ptr, remaining,

                       "{"

                       "\"EventType\":\"%s\","

                       "\"ClientId\":\"%s\","

                       "\"Seq\":%d,", event_type, client_id,

                       seq_nr_value);

    }
    if (len < 0 || len >= remaining) {

        printf("Buffer too short. Have %d, need %d + \\0\n", remaining,

               len);

        return;

    }
    remaining -= len;
    buf_ptr += len;
    len = snprintf(buf_ptr, remaining, "}");
    if (len < 0 || len >= remaining) {

        LOG_ERR("Buffer too short. Have %d, need %d + \\0\n", remaining,

                len);

        return;

    }
    printf("\n\n\nApp Buffer: %s\n\n\n", app_buffer);

    mqtt_publish(&conn, NULL, pub_topic, (uint8_t *) app_buffer, strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
    LOG_DBG("Publish!\n");
}


static void

publish(char *topic) {
    bool running = true;
    while (running) {
        if (mutex_try_lock(&mqtt_mutex)) {
            printf("\n\nPrendo il lock per publish\n\n");
            if (construct_pub_topic(topic) == 0) {
                //Fatal error. Topic larger than the buffer
                printf("State Config Error");
            }
            printf("\nStampo il buffer");
            for (int i = 0; i < messages_length; i++) {
                printf("\n%s", message_to_publish[i][0]);
                if (message_to_publish[i][0] == NULL) {
                    break;
                }
                send_to_mqtt_broker(message_to_publish[i][0], "CONNECTION");
            }
            mutex_unlock(&mqtt_mutex);
            printf("\n\nRilascio il lock per publish\n\n");
            clear_mqtt_buffer();
            running = false;
        }
    }
}

static bool
have_connectivity(void) {
    if (uip_ds6_get_global(ADDR_PREFERRED) == NULL ||
        uip_ds6_defrt_choose() == NULL) {
        return false;
    }
    return true;
}

/*---------------------------------------------------------------------------*/

/* SIMPLE UDP FUNCTIONS*/

static void broadcast_callback(struct simple_udp_connection *c,

                               const uip_ipaddr_t *sender_addr,

                               uint16_t sender_port,

                               const uip_ipaddr_t *receiver_addr,

                               uint16_t receiver_port, const uint8_t *data,

                               uint16_t datalen) {

}


static void broadcast_receiver_callback(struct simple_udp_connection *c,

                                        const uip_ipaddr_t *sender_addr,

                                        uint16_t sender_port,

                                        const uip_ipaddr_t *receiver_addr,

                                        uint16_t receiver_port,

                                        const uint8_t *data, uint16_t datalen) {
    insert_mqtt_buffer((char *) data, "CONNECTION");
    printf("\nReceived %s\n", data);
}

PROCESS_THREAD(broadcast, ev, data) {


    uip_ipaddr_t addr;

    PROCESS_BEGIN();


                etimer_set(&broadcast_timer, CLOCK_SECOND * 20);


                simple_udp_register(&broadcast_connection, 8765, NULL, 5678,

                                    broadcast_callback);


                while (1) {

                    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&broadcast_timer));


                    printf("\nSending broadcast\n");

                    uip_create_linklocal_allnodes_mcast(&addr);

                    simple_udp_sendto(&broadcast_connection, client_id, strlen(client_id), &addr);

                    etimer_reset(&broadcast_timer);

                    printf("Broadcast sent");

                }


    PROCESS_END();

}


PROCESS_THREAD(broadcast_receiver, ev, data) {

    PROCESS_BEGIN();
                simple_udp_register(&broadcast_receiver_connection, 5678, NULL, 8765,

                                    broadcast_receiver_callback);

                while (1) {
                    PROCESS_WAIT_EVENT();
                }


    PROCESS_END();

}

PROCESS_THREAD(mqtt_client_main_process, ev, data) {
    PROCESS_BEGIN();
                LOG_DBG("\n\nMQTT Client Main Process\n");
                update_config();
                def_rt_rssi = 0x8000000;
                uip_icmp6_echo_reply_callback_add(&echo_reply_notification, echo_reply_handler);
                etimer_set(&mqtt_register_timer, 3 * CLOCK_SECOND);
                while (1) {
                    PROCESS_YIELD();
                    if (ev == PROCESS_EVENT_TIMER) {
                        if (data == &mqtt_register_timer) {
                            mqtt_register(&conn, &mqtt_client_main_process, client_id, mqtt_event,
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
                                             30, MQTT_CLEAN_SESSION_ON);
                            } else {
                                etimer_reset(&mqtt_connect_timer);
                            }
                        } else if (data == &mqtt_publish_timer) {
                            publish("motes-connections");
                        }
                    } else if (ev == PROCESS_EVENT_POLL ||
                               (ev == PROCESS_EVENT_TIMER && data == &mqtt_connected_timer)) {
                        if (mqtt_ready(&conn) && conn.out_buffer_sent) {
                            subscribe(MQTT_INFECTION_TOPIC);
                            etimer_set(&mqtt_publish_timer, 5*CLOCK_SECOND);
                        } else {
                            LOG_DBG("Resetting the publish timer\n");
                            etimer_reset(&mqtt_connected_timer);
                        }
                    }


                }
    PROCESS_END();
}