CONTIKI_PROJECT = contact-tracing
all: $(CONTIKI_PROJECT)

MODULES += os/net/app-layer/mqtt
MODULES += os/services/shell
MODULES += os/services/rpl-border-router
MODULES_REL += arch/platform/$(TARGET)
PLATFORMS_ONLY = srf06-cc26xx cc2538dk openmote-cc2538 zoul native simplelink

CONTIKI = /home/sonny/contiki-ng
-include $(CONTIKI)/Makefile.identify-target
include $(CONTIKI)/Makefile.include



