include custom.mk

.PHONY: all clear

LIBUT_DIR:=../libut

USERVER_DIR:=../userver

PROTOCOL_DIR:=./protocols
GM_P:=gm-proto.u
GMC_P:=gmc-proto.u
ADV_P:=adv-proto.u
GB_P:=gb-proto.u

COORD_DIR:=.
COORD:=coordinator
COORD_SRCS:=coordinator.c gb-sender.c gb-receiver.c

NODE_SRCS:=node.c

C_DIR:=../common
C_SRCS:=ansi-utils.c my-pjlib-utils.c

O_DIR:=../object-pool
O_SRCS:=object-pool.c

EP_DIR:=../media-endpoint
EP_SRCS:=endpoint.c

GEN_DIR:=gen
GEN_SRCS:=gm-server.c gm-client.c gmc-server.c gmc-client.c adv-server.c adv-client.c gb-server.c gb-client.c

HT_DIR:=../hash-table
HT_SRCS:=hash-table.c

CFLAGS:=-DPJ_AUTOCONF=1 -O2 -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1 -fms-extensions
CFLAGS+=-I$(LIBS_DIR)/include
CFLAGS+=-I$(LIBS_DIR)/include/json-c
CFLAGS+=-I$(ICS_DIR)/include -I$(Q_DIR)/include -I$(O_DIR)/include
CFLAGS+=-I$(C_DIR)/include
CFLAGS+=-I$(PROTOCOLS_DIR)/include
CFLAGS+=-I$(GEN_DIR)
CFLAGS+=-I$(NODE_DIR)/include
CFLAGS+=-I$(EP_DIR)/include
CFLAGS+=-I$(HT_DIR)/include
CFLAGS+=-D__ICS_INTEL__
CFLAGS+=-Iinclude 
CFLAGS+=-I$(LIBUT_DIR)/include

all: gen-gm gen-gmc gen-adv gen-gb $(COORD) $(NTEST)

gen-gm: $(PROTOCOL_DIR)/$(GM_P)
	mkdir -p gen
	awk -v base_dir=$(USERVER_DIR) -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

gen-gmc: $(PROTOCOL_DIR)/$(GMC_P)
	mkdir -p gen
	awk -v base_dir=$(USERVER_DIR) -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

gen-adv: $(PROTOCOL_DIR)/$(ADV_P)
	mkdir -p gen
	awk -v base_dir=$(USERVER_DIR) -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

gen-gb: $(PROTOCOL_DIR)/$(GB_P)
	mkdir -p gen
	awk -v base_dir=$(USERVER_DIR) -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

$(NTEST): $(NTEST_SRCS:.c=.o) $(NODE_SRCS:.c=.o) $(GEN_SRCS:.c=.o) $(C_SRCS:.c=.o) $(O_SRCS:.c=.o) gb-receiver.o $(EP_SRCS:.c=.o) $(HT_SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)

$(COORD): $(COORD_SRCS:.c=.o) $(GEN_SRCS:.c=.o) $(C_SRCS:.c=.o) $(O_SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(NTEST_SRCS:.c=.o): %.o: test/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(EP_SRCS:.c=.o): %.o: $(EP_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(COORD_SRCS:.c=.o): %.o: $(COORD_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(NODE_SRCS:.c=.o): %.o: src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(GEN_SRCS:.c=.o): %.o: $(GEN_DIR)/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(C_SRCS:.c=.o): %.o: $(C_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(O_SRCS:.c=.o): %.o: $(O_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(HT_SRCS:.c=.o): %.o: $(HT_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)

clean: 
	rm -fr *.o gen/ gen-gm gen-gmc gen-adv gen-gb $(NTEST) $(COORD)
