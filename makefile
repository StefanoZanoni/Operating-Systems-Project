SHELL := /bin/bash

CC := gcc
CFLAGS := -Wall -pedantic -g -std=c99 -O3
LDLIBS := -lpthread
ARFLAGS := rcs
LDFLAGS := -L.
AR := ar

SERVER_SRC := $(wildcard ./src/server/*.c)
CLIENT_SRC := $(wildcard ./src/client/*.c)

.SILENT: server.out client.out clean test1 test2 test3 src/API/api.o

.PHONY: test1 test2 test3 clean

all: server.out client.out

TARGETS := server.out \
		   client.out 

libs/libapi.a: src/API/api.o
	@$(AR) $(ARFLAGS) $@ $<

server.out: $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o server.out $(LDLIBS)

client.out: $(CLIENT_SRC) libs/libapi.a
	$(CC) $(CFLAGS) $(CLIENT_SRC) $(LDFLAGS)/libs -lapi -o client.out

test1: client.out server.out clean
	scripts/test1.sh

test2: client.out server.out clean
	scripts/test2.sh

test3: client.out server.out clean
	scripts/test3.sh

clean:
	rm -f ./src/API/api.o