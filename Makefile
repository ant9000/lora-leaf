APPLICATION = lora-leaf
BOARD ?= samr34-xpro
RIOTBASE ?= $(CURDIR)/../riot
QUIET ?= 1
DEVELHELP ?= 1

STDIO_INTERFACE ?= uart
SLEEP_SECONDS ?= 300
VERBOSE_DEBUG ?= 0
ENABLE_WAKEUP_PIN ?= 0

LORA_REGION ?= EU868
USEPKG += semtech-loramac

USEMODULE += fmt
USEMODULE += shell
USEMODULE += xtimer
USEMODULE += sx1276
USEMODULE += periph_flashpage

ifeq (usb,$(STDIO_INTERFACE))
  USEMODULE += auto_init_usbus
  USEMODULE += stdio_cdc_acm

  # USB device vendor and product ID
  DEFAULT_VID = 1209
  DEFAULT_PID = 0001
  USB_VID ?= $(DEFAULT_VID)
  USB_PID ?= $(DEFAULT_PID)

  CFLAGS += -DUSB_CONFIG_VID=0x$(USB_VID) -DUSB_CONFIG_PID=0x$(USB_PID)
ifneq (0, $(VERBOSE_DEBUG))
  CFLAGS += -DUSBUS_CDC_ACM_STDIO_BUF_SIZE=2048
endif
endif

CFLAGS += -DSLEEP_SECONDS=$(SLEEP_SECONDS)
CFLAGS += -DVERBOSE_DEBUG=$(VERBOSE_DEBUG)
CFLAGS += -DENABLE_WAKEUP_PIN=$(ENABLE_WAKEUP_PIN)
CFLAGS += -DDEVEUI=\"$(DEVEUI)\"
CFLAGS += -DAPPEUI=\"$(APPEUI)\"
CFLAGS += -DAPPKEY=\"$(APPKEY)\"

include $(RIOTBASE)/Makefile.include

ifeq (usb,$(STDIO_INTERFACE))
.PHONY: usb_id_check
usb_id_check:
	@if [ $(USB_VID) = $(DEFAULT_VID) ] || [ $(USB_PID) = $(DEFAULT_PID) ] ; then \
        $(COLOR_ECHO) "$(COLOR_RED)Private testing pid.codes USB VID/PID used!, do not use it outside of test environments!$(COLOR_RESET)" 1>&2 ; \
        $(COLOR_ECHO) "$(COLOR_RED)MUST NOT be used on any device redistributed, sold or manufactured, VID/PID is not unique!$(COLOR_RESET)" 1>&2 ; \
    fi

all: | usb_id_check
endif
