SPIN_APP_NAME = tcp
SPIN_APP_SRCS = \
	handlers/stcp_handlers.c \
	handlers/stcp_rx.c \
	handlers/stcp_tx.c \
	handlers/stcp_utils.c \
	handlers/ooo/multilist.c \
	handlers/ooo/hpool.c \
	handlers/ooo/ooo.c

SPIN_CFLAGS = -O3 -g -flto -I. -Iinclude/ -Ihandlers/ooo/
SPIN_LDFLAGS = -lm
NUM_PACKETS ?= 64

include $(PSPIN_RT)/rules/spin-handlers.mk

.PHONY: driver

all::
	make deploy
	make driver

driver/cmdline.c:
	gengetopt -i driver/tcp_driver.ggo --output-dir=driver/

driver: driver/tcp_driver.cpp driver/cmdline.c
	g++ -g -DHOST -I. -Iinclude/ -Ihandlers/ooo/ -I$(PSPIN_RT)/runtime/include/ -I$(PSPIN_HW)/verilator_model/include driver/tcp_driver.cpp driver/cmdline.c driver/multilist_host.c -L$(PSPIN_HW)/verilator_model/lib/ -lpspin -o tcp_driver -lpthread

driver_debug: driver/tcp_driver.cpp driver/cmdline.c
	g++ -g -DHOST -I. -Iinclude/ -Ihandlers/ooo/ -I$(PSPIN_RT)/runtime/include/ -I$(PSPIN_HW)/verilator_model/include driver/tcp_driver.cpp driver/cmdline.c driver/multilist_host.c -L$(PSPIN_HW)/verilator_model/lib/ -lpspin_debug -o tcp_driver_debug -lpthread
