CFLAGS=-O3 -W -Wall -Wextra -ansi -pedantic -lm
CC=gcc
SOURCES=$(wildcard src/zopfli/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

all: $(OBJECTS)
	ar rcs libzopfli.a $(OBJECTS)

clean:
	rm -f src/zopfli/*.o
	rm -f libzopfli.a