#include "contiki.h"
#include "lib/random.h"
#include "net/ipv6/uip-debug.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip.h"
#include "simple-udp.h"
#include "sys/etimer.h"
#include <stdio.h>
#include <stdlib.h>

static struct simple_udp_connection broadcast_connection,
    broadcast_receiver_connection;

PROCESS(broadcast, "Broadcast Process");
PROCESS(broadcast_receiver, "Broadcast Receiver Process");
AUTOSTART_PROCESSES(&broadcast, &broadcast_receiver);

typedef struct array {
  char **addresses;
  int dimension;
} dynamic_array;

dynamic_array *encountered_devices;
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
  if (encountered_devices->addresses[encountered_devices->dimension] != NULL) {
    encountered_devices->dimension++;
    encountered_devices->addresses =
        realloc(encountered_devices->addresses,
                encountered_devices->dimension * sizeof(char *));
  }
  encountered_devices->addresses[encountered_devices->dimension] = sender_addr;
}

PROCESS_THREAD(broadcast, ev, data) {

  uip_ipaddr_t addr;
  char *message = "ping";
  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, 1234, NULL, 1900,
                      broadcast_callback);
  while (1) {
    printf("Sending broadcast\n");
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&broadcast_connection, message, strlen(message), &addr);
  }

  PROCESS_END();
}

PROCESS_THREAD(broadcast_receiver, ev, data) {
  encountered_devices = calloc(1, sizeof(dynamic_array));
  encountered_devices->dimension = 0;
  encountered_devices->addresses = calloc(1, sizeof(char *));
  PROCESS_BEGIN();
  simple_udp_register(&broadcast_receiver_connection, 1900, NULL, 1234,
                      broadcast_receiver_callback);

  while (1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}