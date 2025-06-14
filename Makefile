CC = gcc
CFLAGS = -Iinclude
CFLAGS += -Wall -Wextra -Wpedantic

ifdef DEBUG
	CFLAGS += -ggdb -O0
else
	CFLAGS += -O1
endif

LIB_SRC = $(wildcard src/lib/*.c)
LIB_OBJ = $(patsubst src/lib/%.c, build/lib/%.o, $(LIB_SRC))

.PHONY: all clean

all: bin/server bin/client bin/test

build/lib/%.o: src/lib/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

bin/server: $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ src/server.c -o $@

bin/client: $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ src/client.c -o $@

clean:
	rm -r bin/* build/*
