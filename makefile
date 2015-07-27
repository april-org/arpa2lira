CFLAGS := $(shell pkg-config --cflags april-ann) -Wall -std=c++11 -O3 -I /usr/local/include/hat-trie
LIBS := $(shell pkg-config --libs april-ann hat-trie-0.1) -lhat-trie

OBJS = src/arpa2lira.o src/binarize_arpa.o src/config.o src/murmur_hash.o

all: bin/arpa2lira

bin/arpa2lira: src/arpa2lira
	mkdir -p bin
	cp -f src/arpa2lira bin

src/arpa2lira: $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LIBS)

%.o: %.cc
	$(CXX) -c $(CFLAGS) $< -o $@

test:
	$(MAKE) -C test

clean:
	rm -f $(OBJS)
	rm -f src/arpa2lira
	rm -f bin/*

.PHONY: all clean
