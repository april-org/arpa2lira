CFLAGS := $(shell pkg-config --cflags april-ann) -O3 -Wall
LIBS := $(shell pkg-config --libs april-ann)

all:
	$(MAKE) -C test

clean:
	$(MAKE) -C test clean

.PHONY: all
