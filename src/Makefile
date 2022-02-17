
CC=$(shell sst-config --CC)
CXX=$(shell sst-config --CXX)
CXXFLAGS=$(shell sst-config --ELEMENT_CXXFLAGS)
LDFLAGS=$(shell sst-config --ELEMENT_LDFLAGS)

# Add hard-coded path to help out CLion
CXXFLAGS += -I$(HOME)/local/include

# Warn if undefined symbols, only works for GCC, not Clang
# LDFLAGS += -Wl,--no-undefined


# Check if we're debugging or not
ifndef PHOLD_DEBUG
  CXXFLAGS_PHOLD_DEBUG=-g -O3
  $(info Building optimized with $(CXXFLAGS_PHOLD_DEBUG))
  CXXFLAGS+=$(CXXFLAGS_PHOLD_DEBUG)
else
  CXXFLAGS_PHOLD_DEBUG=-g -Og -DPHOLD_DEBUG
  $(info Building debug with $(CXXFLAGS_PHOLD_DEBUG))
  CXXFLAGS+=$(CXXFLAGS_PHOLD_DEBUG)
endif

ifdef RNG_DEBUG
  CXXFLAGS_RNG_DEBUG=-DRNG_DEBUG
  $(info Building RNG debug with $(CXXFLAGS_RNG_DEBUG))
  CXXFLAGS+=$(CXXFLAGS_RNG_DEBUG)
endif

# Check if we're skipping rngs
ifdef PHOLD_FIXED
  CXXFLAGS_PHOLD_FIXED=-DPHOLD_FIXED
  $(info Using fixed rng values with $(CXXFLAGS_PHOLD_FIXED))
  CXXFLAGS+=$(CXXFLAGS_PHOLD_FIXED)
endif


SRCDIR = $(PWD)/src
DISTDIR = $(HOME)/local
LIBDIR  = $(DISTDIR)/lib/sst-elements-library

SRCS = $(wildcard *.cc)
HDRS = $(wildcard *.h)
ALL = $(SRCS) $(HDRS)

OBJS = $(SRCS:%.cc=%.o)
LIB  = libphold.so

# Make the build itself verbose
VERBOSE = 0
ifeq ($(VERBOSE),1)
  VERB=
else
  VERB=@
  MAKEFLAGS += --no-print-directory
endif


all: $(LIB)
	@:

# Generate dependency files .d
DEPS = $(SRCS:%.cc=%.d)
DEPFLAGS = -MT $@ -MMD -MP -MF $*.d
%.o : %.cc %.d Makefile
	@echo "CXX" $<
	$(VERB)$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $<

$(DEPS):
ifneq ($(MAKECMDGOALS),clean)
  include $(wildcard $(DEPS))
endif

$(LIB): $(OBJS)
	@echo "LD" $@
	$(VERB)$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

SSTREGCMD = sst-register $(SSTCONFARG)
install: $(LIB)
build-date: $(LIB)
	@echo "SST_REG"
	$(VERB)mkdir -p $(LIBDIR)
	$(VERB)cp $< $(LIBDIR)
	$(VERB)touch $(SSTCONFIG)
	$(VERB)$(SSTREGCMD) phold LIBDIR=$(LIBDIR)
	$(VERB)$(SSTREGCMD) SST_ELEMENT_SOURCE phold=$(SRCDIR)
	$(VERB)$(SSTREGCMD) SST_ELEMENT_TESTS  phold=$(SRCDIR)/tests
	$(VERB)touch $@


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

