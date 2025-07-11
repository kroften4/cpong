CC = gcc
CFLAGS = -Iinclude
CFLAGS += -Wall -Wextra -Wpedantic
LFLAGS = -lSDL3 -lm

ifdef DEBUG
	CFLAGS += -ggdb -O0 -D DEBUG
else
	CFLAGS += -O1
endif

COMMON_SRC = $(wildcard src/common/*.c)
COMMON_OBJ = $(patsubst src/common/%.c, build/common/%.o, $(COMMON_SRC))
LIB_SRC = $(wildcard src/lib/*.c)
LIB_OBJ = $(patsubst src/lib/%.c, build/lib/%.o, $(LIB_SRC))
CLIENT_SRC = $(wildcard src/client/*.c)
CLIENT_OBJ = $(patsubst src/client/%.c, build/client/%.o, $(CLIENT_SRC))
SERVER_SRC = $(wildcard src/server/*.c)
SERVER_OBJ = $(patsubst src/server/%.c, build/server/%.o, $(SERVER_SRC))

.PHONY: all clean

all: bin/server bin/client bin/test

build/lib/%.o: src/lib/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

build/common/%.o: src/common/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

build/server/%.o: src/server/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

build/client/%.o: src/client/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

bin/server: $(LIB_OBJ) $(COMMON_OBJ) $(SERVER_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

bin/client: $(LIB_OBJ) $(COMMON_OBJ) $(CLIENT_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

bin/test: $(LIB_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ test/binarr.c -o $@ $(LFLAGS)

clean:
	rm -r bin/* build/*

