CFLAGS := $(shell pkg-config --cflags april-ann) -Wall -std=c++11 -O3
LIBS := $(shell pkg-config --libs april-ann)

OBJS = binarize_arpa.o config.o murmur_hash.o binarpa2lira.o

all: $(OBJS)

%.o: %.cc
	$(CXX) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS)

.PHONY: all
