CONTIKI_PROJECT = contact-tracing
all: $(CONTIKI_PROJECT)
CONTIKI = /home/sonny/contiki-ng-mw-2021
include $(CONTIKI)/Makefile.include
-include $(CONTIKI)/Makefile.identify-target
MODULES += os/services/shell
MODULES += os/services/rpl-border-router
MODULES_REL += arch/platform/$(TARGET)
MODULES += os/net/app-layer/mqtt
PLATFORMS_ONLY = srf06-cc26xx cc2538dk openmote-cc2538 zoul native simplelink