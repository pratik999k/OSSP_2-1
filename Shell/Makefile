CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -D_POSIX_C_SOURCE=200809L

.PHONY: all clean test

all: forge

SOURCES := $(wildcard src/*.c)

forge: $(SOURCES) src/forge.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SOURCES)

test: forge
	sh tests/test_forge.sh

clean:
	rm -f forge
