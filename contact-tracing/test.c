#include "contiki.h"
#include <stdio.h>

PROCESS(test_proc, "Test Process");
AUTOSTART_PROCESSES(&test_proc);

PROCESS_THREAD(test_proc, ev, data){
    PROCESS_BEGIN();
    printf("Hello world!");
    PROCESS_END();
}