.PHONY: all clear

LIBUT_DIR:=../libut

USERVER_DIR:=../userver

PROTOCOL_DIR:=./protocols
GM_P:=gm-proto.u
GMC_P:=gmc-proto.u
ADV_P:=adv-proto.u
GB_P:=gb-proto.u

NODE_DIR:=.
NODE:=node
NODE_SRCS:=node.c gb-receiver.c

COORD_DIR:=.
COORD:=coordinator
COORD_SRCS:=coordinator.c gb-sender.c

C_DIR:=../common
C_SRCS:=ansi-utils.c

ANSI_O_DIR:=../ansi-opool
ANSI_O_SRCS:=object-pool.c

GEN_DIR:=gen
GEN_SRCS:=gm-server.c gm-client.c gmc-server.c gmc-client.c adv-server.c adv-client.c gb-server.c gb-client.c

NTEST_SRCS:=node-test.c

JSONC_DIR:=../json-c/output

CFLAGS:=-I$(GEN_DIR) -I$(C_DIR)/include -Wall
CFLAGS+=-I$(JSONC_DIR)/include/json-c 
CFLAGS+=-I$(LIBUT_DIR)/include 
CFLAGS+=-I$(O_DIR)/include 
CFLAGS+=-Iinclude -D__ICS_INTEL__
CFLAGS+=-I$(ANSI_O_DIR)/include 
CFLAGS+=-Iinclude

LIBS:=$(JSONC_DIR)/lib/libjson-c.a -lpthread

all: gen-gm gen-gmc gen-adv gen-gb  $(NODE) $(COORD)

gen-gm: $(PROTOCOL_DIR)/$(GM_P)
	mkdir -p gen
	awk -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

gen-gmc: $(PROTOCOL_DIR)/$(GMC_P)
	mkdir -p gen
	awk -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

gen-adv: $(PROTOCOL_DIR)/$(ADV_P)
	mkdir -p gen
	awk -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

gen-gb: $(PROTOCOL_DIR)/$(GB_P)
	mkdir -p gen
	awk -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

$(NODE): $(NODE_SRCS:.c=.o) $(GEN_SRCS:.c=.o) $(C_SRCS:.c=.o) $(NTEST_SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(COORD): $(COORD_SRCS:.c=.o) $(GEN_SRCS:.c=.o) $(C_SRCS:.c=.o) $(ANSI_O_SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(NODE_SRCS:.c=.o): %.o: $(NODE_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(NTEST_SRCS:.c=.o): %.o: $(NODE_DIR)/test/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(COORD_SRCS:.c=.o): %.o: $(COORD_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(GEN_SRCS:.c=.o): %.o: $(GEN_DIR)/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(C_SRCS:.c=.o): %.o: $(C_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(ANSI_O_SRCS:.c=.o): %.o: $(ANSI_O_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)

clean: 
	rm -fr *.o gen/ gen-gm gen-gmc gen-adv gen-gb node coordinator
