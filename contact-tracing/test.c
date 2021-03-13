#include "contiki.h"
#include "simple-udp.h"
#include "http-socket.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(contact_tracing_process, "Contact tracing");
AUTOSTART_PROCESSES(&contact_tracing_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(contact_tracing_process, ev, data)
{
    http_socket_get()
        PROCESS_BEGIN();
    static struct simple_udp_connection udp_conn;
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/