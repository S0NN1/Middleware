CONTIKI_PROJECT = contact-tracing
all: $(CONTIKI_PROJECT)

MODULES += os/services/shell
MODULES_REL += arch/platform/$(TARGET)
MODULES += os/net/app-layer/mqtt
CONTIKI = /home/sonny/contiki-ng
-include $(CONTIKI)/Makefile.identify-target

MODULES_REL += arch/platform/$(TARGET)
include $(CONTIKI)/Makefile.include