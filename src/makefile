CFLAGS := $(shell pkg-config --cflags april-ann ) -Wall -std=c++11 -O3 -I/usr/local/include/hat-trie
LIBS := $(shell pkg-config --libs april-ann hat-trie-0.1)  -L/usr/local/include/hat-trie

OBJS = binarize_arpa.o config.o murmur_hash.o

all: $(OBJS)

%.o: %.cc
	$(CXX) -c $(CFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f $(OBJS)

.PHONY: all
