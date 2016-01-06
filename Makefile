CFLAGS=-O3 -W -Wall -Wextra -lm
CC=gcc
SOURCES=$(wildcard src/*.c)
OBJECTS=$(patsubst src/%.c,build/%.o,$(SOURCES))

TESTFILES = $(wildcard corpus/*.png)

PNGZ=bin/pngz
LIBPNG=build/libpng.a
ZOPFLI=build/libzopfli.a
ZLIB=build/libz.a

all: $(PNGZ)

$(PNGZ): build $(ZLIB) $(LIBPNG) $(ZOPFLI) $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) -L./build -lpng -lz -lzopfli $(CFLAGS)

.PHONY: build
build:
	@mkdir -p bin
	@mkdir -p build
	@mkdir -p test_output

$(ZLIB):
	cd lib/zlib-1.2.8;make distclean;./configure;make;
	cp lib/zlib-1.2.8/libz.a $(ZLIB)

$(LIBPNG): $(ZLIB)
	cd lib/libpng-1.6.20;make clean;./configure;make;
	cp lib/libpng-1.6.20/.libs/libpng16.a $(LIBPNG)

$(ZOPFLI):
	cd lib/zopfli-1.0.1;make clean;make;
	cp lib/zopfli-1.0.1/libzopfli.a $(ZOPFLI)

build/%.o: src/%.c
	$(CC) -iquote./lib/libpng-1.6.20 -iquote./lib/zopfli-1.0.1/src/zopfli \
	-iquote./lib/zlib-1.2.8 -c -o $@ $< $(CFLAGS)

.PHONY: distclean clean

distclean: clean
	rm -f build/*.a
	find lib/ -name "*.a" -type f -delete
	find lib/ -name "*.o" -type f -delete

clean:
	rm -f build/*.o
	rm -f $(PNGZ)
	rm -rf test_output

.PHONY: test
test: build $(TESTFILES)
$(TESTFILES): $(PNGZ)
	./$< $@ test_output/$(notdir $@)

