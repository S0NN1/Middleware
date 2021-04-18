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
#include "simple-udp.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "sys/etimer.h"
#include "working components/mqtt/mqtt-client.h"
#include <string.h>
#include <strings.h>

#include <stdio.h>

#define LOG_MODULE "mqtt-client"
#define LOG_LEVEL LOG_LEVEL_NONE

#ifdef MQTT_CLIENT_CONF_WITH_IBM_WATSON
#define MQTT_CLIENT_WITH_IBM_WATSON MQTT_CLIENT_CONF_WITH_IBM_WATSON
#else
#define MQTT_CLIENT_WITH_IBM_WATSON 0
#endif

#ifdef MQTT_CLIENT_CONF_BROKER_IP_ADDR
#define MQTT_CLIENT_BROKER_IP_ADDR MQTT_CLIENT_CONF_BROKER_IP_ADDR
#else
#define MQTT_CLIENT_BROKER_IP_ADDR "fd00::1"
#endif

#ifdef MQTT_CLIENT_CONF_ORG_ID
#define MQTT_CLIENT_ORG_ID MQTT_CLIENT_CONF_ORG_ID
#else
#define MQTT_CLIENT_ORG_ID "quickstart"
#endif

#ifdef MQTT_CLIENT_CONF_AUTH_TOKEN
#define MQTT_CLIENT_AUTH_TOKEN MQTT_CLIENT_CONF_AUTH_TOKEN
#else
#define MQTT_CLIENT_AUTH_TOKEN "AUTHTOKEN"
#endif

#if MQTT_CLIENT_WITH_IBM_WATSON
static const char *broker_ip = "0064:ff9b:0000:0000:0000:0000:b8ac:7cbd";
#define MQTT_CLIENT_USERNAME "use-token-auth"
#else
/* Without IBM Watson support. To be used with other brokers, e.g. Mosquitto */
static const char *broker_ip = MQTT_CLIENT_BROKER_IP_ADDR;
#ifdef MQTT_CLIENT_CONF_USERNAME
#define MQTT_CLIENT_USERNAME MQTT_CLIENT_CONF_USERNAME
#else
#define MQTT_CLIENT_USERNAME "use-token-auth"
#endif
#endif

#ifdef MQTT_CLIENT_CONF_WITH_EXTENSIONS
#define MQTT_CLIENT_WITH_EXTENSIONS MQTT_CLIENT_CONF_WITH_EXTENSIONS
#else
#define MQTT_CLIENT_WITH_EXTENSIONS 0
#endif

#define STATE_MACHINE_PERIODIC     (CLOCK_SECOND >> 1)
#define RETRY_FOREVER              0xFF
#define RECONNECT_INTERVAL         (CLOCK_SECOND * 2)
#define RECONNECT_ATTEMPTS         RETRY_FOREVER
#define CONNECTION_STABLE_TIME     (CLOCK_SECOND * 5)
#define STATE_INIT            0
#define STATE_REGISTERED      1
#define STATE_CONNECTING      2
#define STATE_CONNECTED       3
#define STATE_PUBLISHING      4
#define STATE_DISCONNECTED    5
#define STATE_NEWCONFIG       6
#define STATE_CONFIG_ERROR 0xFE
#define STATE_ERROR        0xFF
#define CONFIG_ORG_ID_LEN        32
#define CONFIG_TYPE_ID_LEN       32
#define CONFIG_AUTH_TOKEN_LEN    32
#define CONFIG_EVENT_TYPE_ID_LEN 32
#define CONFIG_CMD_TYPE_LEN       8
#define CONFIG_IP_ADDR_STR_LEN   64
/* A timeout used when waiting to connect to a network */
#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)
/* Default configuration values */
#define DEFAULT_TYPE_ID             "mqtt-client"
#define DEFAULT_EVENT_TYPE_ID       "status"
#define DEFAULT_SUBSCRIBE_CMD_TYPE  "+"
#define DEFAULT_BROKER_PORT         1883
#define DEFAULT_PUBLISH_INTERVAL    (30 * CLOCK_SECOND)
#define DEFAULT_KEEP_ALIVE_TIMER    60
#define DEFAULT_RSSI_MEAS_INTERVAL  (CLOCK_SECOND * 30)
#define MQTT_CLIENT_SENSOR_NONE     (void *)0xFFFFFFFF
#define ECHO_REQ_PAYLOAD_LEN   20
#define MAX_TCP_SEGMENT_SIZE    32
#define BUFFER_SIZE 64
#define APP_BUFFER_SIZE 512
#define QUICKSTART "quickstart"

typedef struct mqtt_client_config {
    char org_id[CONFIG_ORG_ID_LEN];
    char type_id[CONFIG_TYPE_ID_LEN];
    char auth_token[CONFIG_AUTH_TOKEN_LEN];
    char event_type_id[CONFIG_EVENT_TYPE_ID_LEN];
    char broker_ip[CONFIG_IP_ADDR_STR_LEN];
    char cmd_type[CONFIG_CMD_TYPE_LEN];
    clock_time_t pub_interval;
    int def_rt_ping_interval;
    uint16_t broker_port;
} mqtt_client_config_t;
static struct simple_udp_connection broadcast_connection,
        broadcast_receiver_connection;
static struct timer connection_life;
static uint8_t connect_attempt;
static uint8_t state;
static char client_id[BUFFER_SIZE];
static char pub_topic[BUFFER_SIZE];
static char sub_topic[BUFFER_SIZE];
static struct mqtt_connection conn;
static char app_buffer[APP_BUFFER_SIZE];
enum action {
    CONNECTION, ALERT
};
static struct mqtt_message *msg_ptr = 0;
static struct etimer publish_periodic_timer;
static char *buf_ptr;
static uint16_t seq_nr_value = 0;
static struct uip_icmp6_echo_reply_notification echo_reply_notification;
static struct etimer echo_request_timer;
static struct etimer connection_to_kafka_timer;
static struct etimer alert_to_kafka_timer;
static int def_rt_rssi = 0;
static mqtt_client_config_t conf;


PROCESS(mqtt_client_setup, "MQTT Client Setup");

PROCESS(mqtt_client_publish, "MQTT Client Publish");

PROCESS(broadcast, "Broadcast Process");

PROCESS(broadcast_receiver, "Broadcast Receiver Process");
AUTOSTART_PROCESSES(&mqtt_client_setup);


/* MQTT CLIENT FUNCTIONS */
static void
echo_reply_handler(uip_ipaddr_t *source, uint8_t ttl, uint8_t *data,
                   uint16_t datalen) {
    if (uip_ip6addr_cmp(source, uip_ds6_defrt_choose())) {
        def_rt_rssi = sicslowpan_get_last_rssi();
    }
}

static void
pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk,
            uint16_t chunk_len) {
    LOG_DBG("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic,
            topic_len, chunk_len);

    if (topic_len != 23 || chunk_len != 1) {
        LOG_ERR("Incorrect topic or chunk len. Ignored\n");
        return;
    }

    if (strncmp(&topic[topic_len - 4], "json", 4) != 0) {
        LOG_ERR("Incorrect format\n");
    }
}

static void
mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data) {
    switch (event) {
        case MQTT_EVENT_CONNECTED: {
            LOG_DBG("Application has a MQTT connection\n");
            timer_set(&connection_life, CONNECTION_STABLE_TIME);
            state = STATE_CONNECTED;
            break;
        }
        case MQTT_EVENT_DISCONNECTED: {
            LOG_DBG("MQTT Disconnect. Reason %u\n", *((mqtt_event_t *) data));

            state = STATE_DISCONNECTED;
            process_poll(&mqtt_client_setup);
            break;
        }
        case MQTT_EVENT_PUBLISH: {
            msg_ptr = data;
            //TODO PUBLISH
            if (msg_ptr->first_chunk) {
                msg_ptr->first_chunk = 0;
                LOG_DBG("Application received publish for topic '%s'. Payload "
                        "size is %i bytes.\n", msg_ptr->topic, msg_ptr->payload_length);
            }
            printf("%s", msg_ptr->topic);

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

static int
construct_sub_topic(char *topic) {
    int len = snprintf(sub_topic, BUFFER_SIZE, "%s", topic);

    /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */
    if (len < 0 || len >= BUFFER_SIZE) {
        LOG_INFO("Sub Topic: %d, Buffer %d\n", len, BUFFER_SIZE);
        return 0;
    }

    return 1;
}

static int
construct_client_id(void) {
    int len = snprintf(client_id, BUFFER_SIZE, "d:%s:%s:%02x%02x%02x%02x%02x%02x",
                       conf.org_id, conf.type_id,
                       linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
                       linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
                       linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);

    if (len < 0 || len >= BUFFER_SIZE) {
        LOG_ERR("Client ID: %d, Buffer %d\n", len, BUFFER_SIZE);
        return 0;
    }

    return 1;
}

static void
update_config(void) {
    if (construct_client_id() == 0) {
        LOG_ERR("State config error\n");
    }
    if (construct_sub_topic("#") == 0) {
        LOG_ERR("State config error\n");
    }
    seq_nr_value = 0;
    state = STATE_INIT;
    etimer_set(&publish_periodic_timer, 0);
}

static int
init_config() {
    memset(&conf, 0, sizeof(mqtt_client_config_t));

    memcpy(conf.org_id, MQTT_CLIENT_ORG_ID, strlen(MQTT_CLIENT_ORG_ID));
    memcpy(conf.type_id, DEFAULT_TYPE_ID, strlen(DEFAULT_TYPE_ID));
    memcpy(conf.auth_token, MQTT_CLIENT_AUTH_TOKEN,
           strlen(MQTT_CLIENT_AUTH_TOKEN));
    memcpy(conf.event_type_id, DEFAULT_EVENT_TYPE_ID,
           strlen(DEFAULT_EVENT_TYPE_ID));
    memcpy(conf.broker_ip, broker_ip, strlen(broker_ip));
    memcpy(conf.cmd_type, DEFAULT_SUBSCRIBE_CMD_TYPE, 1);

    conf.broker_port = DEFAULT_BROKER_PORT;
    conf.pub_interval = DEFAULT_PUBLISH_INTERVAL;
    conf.def_rt_ping_interval = DEFAULT_RSSI_MEAS_INTERVAL;

    return 1;
}

static void
subscribe(char *topic) {
    mqtt_status_t status;

    status = mqtt_subscribe(&conn, NULL, topic, MQTT_QOS_LEVEL_0);

    LOG_DBG("Subscribing!\n");
    if (status == MQTT_STATUS_OUT_QUEUE_FULL) {
        LOG_ERR("Tried to subscribe but command queue was full!\n");
    }
}

static void
publish(char *topic, enum action action, char *receiver_address) {
    if (construct_pub_topic(topic) == 0) {
        /* Fatal error. Topic larger than the buffer */
        printf("State Config Error");
    }
    int len;
    int remaining = APP_BUFFER_SIZE;
    seq_nr_value++;
    char *event_type;
    buf_ptr = app_buffer;
    if (action == CONNECTION) {
        event_type = "CONNECTION";
        len = snprintf(buf_ptr, remaining,
                       "{"
                       "\"EventType\":\"%s\","
                       "\"ClientId\":\"%s\","
                       "\"IpAddressMote\":\"%s\","
                       "\"Seq\":%d,", event_type, client_id, receiver_address,
                       seq_nr_value);
    } else {
        event_type = "ALERT";
        len = snprintf(buf_ptr, remaining,
                       "{"
                       "\"EventType\":\"%s\","
                       "\"ClientId\":\"%s\","
                       "\"Seq\":%d,", event_type, client_id,
                       seq_nr_value);
    }

    if (len < 0 || len >= remaining) {
        LOG_ERR("Buffer too short. Have %d, need %d + \\0\n", remaining,
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

    mqtt_publish(&conn, NULL, pub_topic, (uint8_t *) app_buffer,
                 strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);

    LOG_DBG("Publish!\n");
}

static void
connect_to_broker(void) {
    mqtt_connect(&conn, conf.broker_ip, conf.broker_port,
                 conf.pub_interval * 3);

    state = STATE_CONNECTING;
}

static void
ping_parent(void) {
    if (uip_ds6_get_global(ADDR_PREFERRED) == NULL) {
        return;
    }

    uip_icmp6_send(uip_ds6_defrt_choose(), ICMP6_ECHO_REQUEST, 0,
                   ECHO_REQ_PAYLOAD_LEN);
}

static void
state_machine(void) {
    switch (state) {
        case STATE_INIT:
            /* If we have just been configured register MQTT connection */
            mqtt_register(&conn, &mqtt_client_setup, client_id, mqtt_event,
                          MAX_TCP_SEGMENT_SIZE);
            if (strncasecmp(conf.org_id, QUICKSTART, strlen(conf.org_id)) != 0) {
                if (strlen(conf.auth_token) == 0) {
                    LOG_ERR("User name set, but empty auth token\n");
                    state = STATE_ERROR;
                    break;
                } else {
                    mqtt_set_username_password(&conn, MQTT_CLIENT_USERNAME,
                                               conf.auth_token);
                }
            }
            conn.auto_reconnect = 0;
            connect_attempt = 1;
            state = STATE_REGISTERED;
            LOG_DBG("Init\n");
            /* Continue */
        case STATE_REGISTERED:
            if (uip_ds6_get_global(ADDR_PREFERRED) != NULL) {
                LOG_DBG("Registered. Connect attempt %u\n", connect_attempt);
                ping_parent();
                connect_to_broker();
            }
            etimer_set(&publish_periodic_timer, NET_CONNECT_PERIODIC);
            return;
            break;
        case STATE_CONNECTING:
            LOG_DBG("Connecting (%u)\n", connect_attempt);
            break;
        case STATE_CONNECTED:
            if (strncasecmp(conf.org_id, QUICKSTART, strlen(conf.org_id)) == 0) {
                LOG_DBG("Using 'quickstart': Skipping subscribe\n");
                state = STATE_PUBLISHING;
            }
            process_start(&mqtt_client_publish, NULL);
            process_start(&broadcast, NULL);
            process_start(&broadcast_receiver, NULL);
        case STATE_DISCONNECTED:
            LOG_DBG("Disconnected\n");
            if (connect_attempt < RECONNECT_ATTEMPTS ||
                RECONNECT_ATTEMPTS == RETRY_FOREVER) {
                /* Disconnect and backoff */
                clock_time_t interval;
                mqtt_disconnect(&conn);
                connect_attempt++;

                interval = connect_attempt < 3 ? RECONNECT_INTERVAL << connect_attempt :
                           RECONNECT_INTERVAL << 3;

                LOG_DBG("Disconnected. Attempt %u in %lu ticks\n", connect_attempt, interval);

                etimer_set(&publish_periodic_timer, interval);

                state = STATE_REGISTERED;
                return;
            } else {
                state = STATE_ERROR;
                LOG_DBG("Aborting connection after %u attempts\n", connect_attempt - 1);
            }
            break;
        case STATE_CONFIG_ERROR:
            LOG_ERR("Bad configuration.\n");
            return;
        case STATE_ERROR:
        default:
            LOG_ERR("Default case: State=0x%02x\n", state);
            return;
    }

    etimer_set(&publish_periodic_timer, STATE_MACHINE_PERIODIC);
}
/* SIMPLE UDP FUNCTIONS*/
static void broadcast_callback(struct simple_udp_connection *c,
                               const uip_ipaddr_t *sender_addr,
                               uint16_t sender_port,
                               const uip_ipaddr_t *receiver_addr,
                               uint16_t receiver_port, const uint8_t *data,
                               uint16_t datalen) {
    printf("kek");
}

static void broadcast_receiver_callback(struct simple_udp_connection *c,
                                        const uip_ipaddr_t *sender_addr,
                                        uint16_t sender_port,
                                        const uip_ipaddr_t *receiver_addr,
                                        uint16_t receiver_port,
                                        const uint8_t *data, uint16_t datalen) {
    printf("%s", data);
}

PROCESS_THREAD(broadcast, ev, data) {
    static struct etimer et;
    uip_ipaddr_t addr;
    PROCESS_BEGIN();
                etimer_set(&et, CLOCK_SECOND*10);

                simple_udp_register(&broadcast_connection, 1234, NULL, 1900,
                                    broadcast_callback);
                while (1) {
                    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

                    printf("\nSending broadcast\n");
                    uip_create_linklocal_allnodes_mcast(&addr);
                    simple_udp_sendto(&broadcast_connection, client_id, strlen(client_id), &addr);
                    etimer_reset(&et);
                }

    PROCESS_END();
}

PROCESS_THREAD(broadcast_receiver, ev, data) {
    PROCESS_BEGIN();
                simple_udp_register(&broadcast_receiver_connection, 1900, NULL, 1234,
                                    broadcast_receiver_callback);

                while (1) {
                    PROCESS_WAIT_EVENT();
                }

    PROCESS_END();
}

PROCESS_THREAD(mqtt_client_setup, ev, data) {

    PROCESS_BEGIN();

                printf("MQTT Client Process\n");

                if (init_config() != 1) {
                    PROCESS_EXIT();
                }
                update_config();
                def_rt_rssi = 0x8000000;
                uip_icmp6_echo_reply_callback_add(&echo_reply_notification,
                                                  echo_reply_handler);
                etimer_set(&echo_request_timer, conf.def_rt_ping_interval);
                while (1) {

                    PROCESS_YIELD();

                    if ((ev == PROCESS_EVENT_TIMER && data == &publish_periodic_timer) ||
                        ev == PROCESS_EVENT_POLL) {
                        state_machine();
                    }

                    if (ev == PROCESS_EVENT_TIMER && data == &echo_request_timer) {
                        ping_parent();
                        etimer_set(&echo_request_timer, conf.def_rt_ping_interval);
                    }
                }

    PROCESS_END();
}

PROCESS_THREAD(mqtt_client_publish, ev, data) {
    PROCESS_BEGIN();
                if (state == STATE_PUBLISHING) {
                    if (timer_expired(&connection_life)) {
                        connect_attempt = 0;
                    }

                    if (mqtt_ready(&conn) && conn.out_buffer_sent) {
                        if (state == STATE_CONNECTED) {
                            subscribe("#");
                            PROCESS_YIELD();
                            if (ev == PROCESS_EVENT_TIMER && data == &connection_to_kafka_timer) {
                                publish("motes-connections", CONNECTION, "192.168.1.1");
                                LOG_DBG("Publishing connection \n");
                            } else if ((ev == PROCESS_EVENT_TIMER && data == &alert_to_kafka_timer)) {
                                publish("motes-connections", ALERT, "192.168.1.1");
                            }
                        }
                        etimer_set(&publish_periodic_timer, conf.pub_interval);
                    } else {
                        LOG_DBG("Publishing... (MQTT state=%d, q=%u)\n", conn.state,
                                conn.out_queue_full);
                    }
                    break;
                }

    PROCESS_END();
}

