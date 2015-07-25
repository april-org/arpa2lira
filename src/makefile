CFLAGS := $(shell pkg-config --cflags april-ann) -Wall -std=c++11 -O3 -I /usr/local/include/hat-trie
LIBS := $(shell pkg-config --libs april-ann hat-trie-0.1) -lhat-trie

OBJS = arpa2lira.o binarize_arpa.o config.o murmur_hash.o

all: arpa2lira

arpa2lira: $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LIBS)

%.o: %.cc
	$(CXX) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS)

.PHONY: all
