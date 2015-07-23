CFLAGS := $(shell pkg-config --cflags april-ann) -std=c++11 -O3 -Wall -lhat-trie
LIBS := $(shell pkg-config --libs april-ann hat-trie-0.1)  -L/usr/local/include/hat-trie

all:
	$(MAKE) -C test

clean:
	$(MAKE) -C test clean

.PHONY: all
