CONTIKI_PROJECT = mqtt-client
all: $(CONTIKI_PROJECT)

MODULES += os/services/shell
MODULES_REL += arch/platform/$(TARGET)
MODULES += os/net/app-layer/mqtt
CONTIKI = /home/sonny/contiki-ng-mw-2021
-include $(CONTIKI)/Makefile.identify-target

MODULES_REL += arch/platform/$(TARGET)

PLATFORMS_ONLY = srf06-cc26xx cc2538dk openmote-cc2538 zoul native simplelink

include $(CONTIKI)/Makefile.include