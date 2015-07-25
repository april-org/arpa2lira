CFLAGS := $(shell pkg-config --cflags april-ann) -Wall -std=c++11 -O3 -I /usr/local/include/hat-trie
LIBS := $(shell pkg-config --libs april-ann hat-trie-0.1) -lhat-trie

all: bin/arpa2lira

bin/arpa2lira: src/arpa2lira src/*.h src/*.cc
	$(MAKE) -C src
	mkdir -p bin
	cp -f src/arpa2lira bin

test:
	$(MAKE) -C test

clean:
	$(MAKE) -C test clean
	rm -f bin/*

.PHONY: all
