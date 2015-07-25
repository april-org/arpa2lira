CFLAGS := $(shell pkg-config --cflags april-ann) -Wall -std=c++11 -O3 -I /usr/local/include/hat-trie
LIBS := $(shell pkg-config --libs april-ann hat-trie-0.1) -lhat-trie

SRC := ../src

OBJS := $(SRC)/binarize_arpa.o $(SRC)/config.o $(SRC)/murmur_hash.o

all: test_binarize_arpa

test_binarize_arpa:
	$(MAKE) -C ../src
	$(CXX) $(OBJS) -o test_binarize_arpa test_binarize_arpa.cc $(CFLAGS) $(LIBS)

clean:
	$(MAKE) -C ../src clean
	rm -f test_binarize_arpa

.PHONY: all test_binarize_arpa
