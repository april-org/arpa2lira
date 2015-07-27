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
