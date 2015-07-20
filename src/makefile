CFLAGS := $(shell pkg-config --cflags april-ann) -Wall -std=c++11
LIBS := $(shell pkg-config --libs april-ann)

OBJS = binarize_arpa.o murmur_hash.o

all: $(OBJS)

binarize_arpa.o: binarize_arpa.cc binarize_arpa.h
	$(CXX) -c -o $@ $< $(CFLAGS)

murmur_hash.o: murmur_hash.cc murmur_hash.h
	$(CXX) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(OBJS)

.PHONY: all
