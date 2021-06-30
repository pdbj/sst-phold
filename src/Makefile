
CXX=$(shell sst-config --CXX)
CXXFLAGS=$(shell sst-config --ELEMENT_CXXFLAGS)
LDFLAGS=$(shell sst-config --ELEMENT_LDFLAGS)

CXXFLAGS+="-I${HOME}/local/include"

all: libphold.so install

DEPS = Phold.cc Phold.h

libphold.so: $(DEPS) Makefile
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

.PHONY: all install clean

install: build-date

build-date: libphold.so
	sst-register phold phold_LIBDIR=$(CURDIR)
	sst-register SST_ELEMENT_SOURCE phold=$(CURDIR)
	sst-register SST_ELEMENT_TESTS  phold=$(CURDIR)/../tests
	touch $@

clean:
	rm -f *.o libphold.so
