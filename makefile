CFLAGS := $(shell pkg-config --cflags april-ann) -std=c++11 -O3 -Wall
LIBS := $(shell pkg-config --libs april-ann)

all:
	$(MAKE) -C test

clean:
	$(MAKE) -C test clean

.PHONY: all
