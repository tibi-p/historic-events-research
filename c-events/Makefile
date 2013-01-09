.PHONY : clean

all: build

build:
	$(MAKE) -C src/c
	$(MAKE) -C src/cpp

rebuild: clean build

clean:
	$(MAKE) -C src/c clean
	$(MAKE) -C src/cpp clean
