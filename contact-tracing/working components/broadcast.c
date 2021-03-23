#include "contiki.h"
#include "lib/random.h"
#include "sys/etimer.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-debug.h"
#include "simple-udp.h"
#include <stdio.h>

static struct simple_udp_connection broadcast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(broadcast, "Broadcast Process");
AUTOSTART_PROCESSES(&broadcast);
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
    printf(" on port %d from port %d with length %d:  %s \n",
           receiver_port, sender_port, datalen, data);
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(broadcast, ev, data)
{

    uip_ipaddr_t addr;

    PROCESS_BEGIN();

    simple_udp_register(&broadcast_connection, 1234,
                        NULL, 1900,
                        receiver);
    while (1)
    {
        printf("Sending broadcast\n");
        uip_create_linklocal_allnodes_mcast(&addr);
        simple_udp_sendto(&broadcast_connection, table.id, sizeof(int), &addr);
    }

    PROCESS_END();
}