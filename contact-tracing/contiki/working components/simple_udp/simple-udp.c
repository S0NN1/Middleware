#include "contiki.h"
#include "lib/random.h"
#include "net/ipv6/uip-debug.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip.h"
#include "simple-udp.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "sys/etimer.h"
#include <stdio.h>

#define KAFKA_ADDRESS "localhost:9091"
static struct simple_udp_connection broadcast_connection,
    broadcast_receiver_connection, socket_connection;

PROCESS(broadcast, "Broadcast Process");
PROCESS(broadcast_receiver, "Broadcast Receiver Process");
AUTOSTART_PROCESSES(&broadcast, &broadcast_receiver);

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
  char message = strcat(id, id2);
  simple_udp_sendto(&socket_connection, message, strlen(message),
                    KAFKA_ADDRESS);
}

static void socket_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port, const uint8_t *data,
                            uint16_t datalen) {
  printf("Connected to Kafka");
}

PROCESS_THREAD(broadcast, ev, data) {
  static struct etimer et;
  uip_ipaddr_t addr;
  char *message = "ping";
  PROCESS_BEGIN();
  etimer_set(&et, CLOCK_SECOND);

  simple_udp_register(&broadcast_connection, 1234, NULL, 1900,
                      broadcast_callback);
  simple_udp_register(&socket_connection, 1234, NULL, 1900, socket_callback);
  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    printf("\nSending broadcast\n");
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&broadcast_connection, message, strlen(message), &addr);
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