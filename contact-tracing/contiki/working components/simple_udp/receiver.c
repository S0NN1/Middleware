#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"

#include "simple-udp.h"
#include "net/ipv6/uip-debug.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 1900

static struct simple_udp_connection broadcast_receiver_connection;

/*---------------------------------------------------------------------------*/
PROCESS(broadcast_receiver, "UDP broadcast example process");
AUTOSTART_PROCESSES(&broadcast_receiver);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
    printf("Data received from ");
    uip_debug_ipaddr_print(sender_addr);
    printf(" on port %d from port %d with length %d: %s \n",
           receiver_port, sender_port, datalen, data);
    /* this line should work,right?*/
    simple_udp_sendto(&broadcast_receiver_connection, "hello", 5, sender_addr);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_receiver, ev, data)
{

    PROCESS_BEGIN();
    simple_udp_register(&broadcast_receiver_connection, 1900,
                        NULL, 1234,
                        receiver);

    while (1)
    {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/