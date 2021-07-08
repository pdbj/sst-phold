
CXX=$(shell sst-config --CXX)
CXXFLAGS=$(shell sst-config --ELEMENT_CXXFLAGS)
LDFLAGS=$(shell sst-config --ELEMENT_LDFLAGS)

# Add hard-coded path to help out CLion
CXXFLAGS+=-I${HOME}/local/include

DISTDIR = ~/local/lib/sst-elements-library

all: install
	@:

SRCS = Phold.cc
HDRS = Phold.h PholdEvent.h
ALL = $(SRCS) $(HDRS)

OBJS = $(SRCS:%.cc=%.o)

# Generate dependency files .d
DEPS = $(SRCS:%.cc=%.d)
DEPFLAGS = -MT $@ -MMD -MP -MF $*.d
%.o : %.cc
%.o : %.cc %.d Makefile
	$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $<

$(DEPS):
include $(wildcard $(DEPS))

libphold.so: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<


build-date: libphold.so
	sst-register phold phold_LIBDIR=$(CURDIR)
	sst-register phold SST_ELEMENT_SOURCE=$(CURDIR)
	sst-register phold SST_ELEMENT_TESTS=$(CURDIR)/../tests
	cp $< $(DISTDIR)/
	touch $@


.PHONY: all install clean info

install: build-date

clean:
	rm -f *.o *.d *.so build-date

info:
	@echo "SRCS:         $(SRCS)"
	@echo "HDRS:         $(HDRS)"
	@echo "ALL:          $(ALL)"
	@echo "OBJS:         $(OBJS)"
	@echo "DEPS:         $(DEPS)"
	@echo "CXX:          $(CXX)"
	@echo "CXXFLAGS:     $(CXXFLAGS)"
	@echo "DEPFLAGS:     $(DEPFLAGS)"
	@echo "LDFLAGS:      $(LDFLAGS)"
