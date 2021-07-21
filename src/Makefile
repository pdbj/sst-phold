
CC=$(shell sst-config --CC)
CXX=$(shell sst-config --CXX)
CXXFLAGS=$(shell sst-config --ELEMENT_CXXFLAGS)
LDFLAGS=$(shell sst-config --ELEMENT_LDFLAGS)

# Add hard-coded path to help out CLion
CXXFLAGS += -I$(HOME)/local/include

# Warn if undefined symbols
# LDFLAGS += -Wl,--no-undefined


# Check if we're debugging or not
ifndef PHOLD_DEBUG
  CXXFLAGS+=-g -O3
  $(info Building optimized)
else
  CXXFLAGS+=-g -Og -DPHOLD_DEBUG
  $(info Building debug)
endif

SRCDIR = $(PWD)/src
DISTDIR = $(HOME)/local
LIBDIR  = $(DISTDIR)/lib/sst-elements-library

all: install
	@:

SRCS = Phold.cc
HDRS = Phold.h PholdEvent.h
ALL = $(SRCS) $(HDRS)

OBJS = $(SRCS:%.cc=%.o)
LIB=libphold.so

# Generate dependency files .d
DEPS = $(SRCS:%.cc=%.d)
DEPFLAGS = -MT $@ -MMD -MP -MF $*.d
%.o : %.cc %.d Makefile
	$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $<

$(DEPS):
include $(wildcard $(DEPS))

$(LIB): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

build-date: $(LIB)
	mkdir -p $(LIBDIR)
	cp $< $(LIBDIR)
#	sst-register phold phold_SOURCE_ROOT=$(SRCDIR)/..
#	sst-register phold phold_HOME=$(DISTDIR)
#	sst-register phold phold_BINDIR=$(DISTDIR)/bin
	sst-register phold phold_LIBDIR=$(LIBDIR)
#	sst-register phold phold_BUILDDIR=$(SRCDIR)/..
	sst-register SST_ELEMENT_SOURCE phold=$(SRCDIR)
	sst-register SST_ELEMENT_TESTS phold=$(SRCDIR)/tests
	touch $@


.PHONY: all install clean info

install: build-date

clean:
	rm -f *.o *.d *.so build-date

info:
	@echo "PWD:          $(PWD)"
	@echo "CURDIR:       $(CURDIR)"
	@echo "SRCDIR:       $(SRCDIR)"
	@echo "SRCS:         $(SRCS)"
	@echo "HDRS:         $(HDRS)"
	@echo "ALL:          $(ALL)"
	@echo "OBJS:         $(OBJS)"
	@echo "LIB:          $(LIB)"
	@echo "DEPS:         $(DEPS)"
	@echo "CXX:          $(CXX)"
	@echo "CXXFLAGS:     $(CXXFLAGS)"
	@echo "DEPFLAGS:     $(DEPFLAGS)"
	@echo "LDFLAGS:      $(LDFLAGS)"
