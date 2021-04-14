#include "contiki.h"

# include "os/sys/log.h"
#include "mqtt.h"
#include "simple-udp.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define LOG_MODULE "mqtt-client"
#define LOG_LEVEL LOG_LEVEL_NONE
/*------SIMPLE-UDP STATIC SECTION--------------------------------------------*/
static struct simple_udp_connection broadcast_connection,
        broadcast_receiver_connection;

static struct mqtt_connection conn;
static struct mqtt_message *msg_ptr = 0;
char *client_id;
static uint16_t BROKER_PORT = 1883;

//static struct mqtt_message *msg_ptr = 0;

#define APP_BUFFER_SIZE 512
static char app_buffer[APP_BUFFER_SIZE];

/*------PROCESS SECTION------------------------------------------------------*/

PROCESS(broadcast,
        "Broadcast Process");

PROCESS(broadcast_receiver,
        "Broadcast Receiver Process");

PROCESS(mqtt_client_process,
        "MQTT Process");
AUTOSTART_PROCESSES(&broadcast, &broadcast_receiver, &mqtt_client_process);
/*------SIMPLE-UDP FUNCTION SECTION------------------------------------------*/
char *PUBLISH_TOPIC = "kek";
static clock_time_t PUBLISH_INTERVAL = (30 * CLOCK_SECOND);
char *SUBSCRIBE_TOPIC = "lmao";

/**
 * @brief Broadcast callback function, called when data is returned from the
 * destination udp mote.
 *
 * @param c simple_udp_connection struct (defined in "simple-udp.c")
 * @param sender_addr sender address
 * @param sender_port sender port
 * @param receiver_addr host address
 * @param receiver_port host port
 * @param data data received
 * @param datalen data length
 */
static void broadcast_callback(struct simple_udp_connection *c,
                               const uip_ipaddr_t *sender_addr,
                               uint16_t sender_port,
                               const uip_ipaddr_t *receiver_addr,
                               uint16_t receiver_port, const uint8_t *data,
                               uint16_t datalen) {
    printf("kek");
}

/**
 * @brief Broadcast receiver callback function, called when data is returned
 * from the destination udp mote.
 *
 * @param c simple_udp_connection struct (defined in "simple-udp.c")
 * @param sender_addr sender address
 * @param sender_port sender port
 * @param receiver_addr host address
 * @param receiver_port host port
 * @param data data received
 * @param datalen data length
 */
static void broadcast_receiver_callback(struct simple_udp_connection *c,
                                        const uip_ipaddr_t *sender_addr,
                                        uint16_t sender_port,
                                        const uip_ipaddr_t *receiver_addr,
                                        uint16_t receiver_port,
                                        const uint8_t *data, uint16_t datalen) {
    char id[UIPLIB_IPV6_MAX_STR_LEN];
    uiplib_ipaddr_snprint(id, sizeof(id), sender_addr);
    char id2[UIPLIB_IPV6_MAX_STR_LEN];
    uiplib_ipaddr_snprint(id, sizeof(id2), receiver_addr);
    char *message = strcat(id, id2);
    printf("%s", message);
}


static void
mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data) {
    switch (event) {
        case MQTT_EVENT_CONNECTED: {
            LOG_DBG("Application has a MQTT connection\n");
            break;
        }
        case MQTT_EVENT_DISCONNECTED: {
            LOG_DBG("MQTT Disconnect. Reason %u\n", *((mqtt_event_t *) data));
            break;
        }
        case MQTT_EVENT_PUBLISH: {
            msg_ptr = data;
            if (msg_ptr->first_chunk) {
                msg_ptr->first_chunk = 0;
                LOG_DBG("Application received publish for topic '%s'. Payload "
                        "size is %i bytes.\n",
                        msg_ptr->topic, msg_ptr->payload_length);
            }
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


PROCESS_THREAD(broadcast, ev, data) {
    static struct etimer et;
    uip_ipaddr_t addr;
    char *message = "ping";

    PROCESS_BEGIN();

                etimer_set(&et, CLOCK_SECOND);

                simple_udp_register(&broadcast_connection, 1234, NULL, 1900,
                                    broadcast_callback);
                while (1) {
                    PROCESS_WAIT_EVENT_UNTIL (etimer_expired(

                            &et));

                    printf("\nSending broadcast\n");
                    uip_create_linklocal_allnodes_mcast(&addr);
                    simple_udp_sendto(&broadcast_connection, message,
                                      strlen(message),
                                      &addr);
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

PROCESS_THREAD(mqtt_client_process, ev, data) {

    PROCESS_BEGIN();
                mqtt_status_t status, status1, status2;
                client_id = "quickstart";
                printf("MQTT Client Process\n");
                status = mqtt_register(&conn, &mqtt_client_process, client_id, mqtt_event,
                                       32);
                status1 = mqtt_connect(&conn, "51.103.25.211", BROKER_PORT, PUBLISH_INTERVAL);
                status2 = mqtt_publish(&conn, NULL, PUBLISH_TOPIC, (uint8_t *) app_buffer,
                                       strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_ON);
                if (status == MQTT_STATUS_OK) {
                    printf("status ok\n");
                }
                if (status1 == MQTT_STATUS_OK) {
                    printf("status1 ok\n");
                }
                if (status2 == MQTT_STATUS_OK) {
                    printf("status2 ok\n");
                }
                if (status != MQTT_STATUS_OK) {
                    printf("status not ok\n");
                }
                if (status1 != MQTT_STATUS_OK) {
                    printf("status1 not ok\n");
                }
                if (status2 != MQTT_STATUS_OK) {
                    printf("status2 not ok\n");
                }
                mqtt_subscribe(&conn, NULL, SUBSCRIBE_TOPIC, MQTT_QOS_LEVEL_0);
    PROCESS_END();

}