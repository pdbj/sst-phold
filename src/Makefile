
SRCDIR = $(PWD)/src
LIBDIR  = $(PWD)/lib

# Check for custom build config resulting from successful install
SSTCONFARG =
SSTCONFIG = $(LIBDIR)/sst-phold.conf
ifneq (,$(wildcard $(SSTCONFIG)))
  SSTCONFARG = --lib-path=$(SSTCONFIG)
endif

CC=$(shell       sst-config $(SSTCONFARG) --CC)
CXX=$(shell      sst-config $(SSTCONFARG) --CXX)
CXXFLAGS=$(shell sst-config $(SSTCONFARG) --ELEMENT_CXXFLAGS)
LDFLAGS=$(shell  sst-config $(SSTCONFARG) --ELEMENT_LDFLAGS)

# Add hard-coded path to help out CLion
#CXXFLAGS += -I$(HOME)/local/include

# Warn if undefined symbols, only works for GCC, not Clang
# LDFLAGS += -Wl,--no-undefined


# Set various build flags
ifneq ($(MAKECMDGOALS),clean)
  PHOLD_DEBUG = 0
  ifeq ($(PHOLD_DEBUG),0)
    CXXFLAGS_PHOLD_DEBUG=-g -O3
    $(info Building optimized with $(CXXFLAGS_PHOLD_DEBUG))
    CXXFLAGS+=$(CXXFLAGS_PHOLD_DEBUG)
  else
    CXXFLAGS_PHOLD_DEBUG=-g -Og -DPHOLD_DEBUG
    $(info Building debug with $(CXXFLAGS_PHOLD_DEBUG))
    CXXFLAGS+=$(CXXFLAGS_PHOLD_DEBUG)
  endif

  RNG_DEBUG = 0
  ifeq ($(RNG_DEBUG),1)
    CXXFLAGS_RNG_DEBUG=-DRNG_DEBUG
    $(info Building RNG debug with $(CXXFLAGS_RNG_DEBUG))
    CXXFLAGS+=$(CXXFLAGS_RNG_DEBUG)
  endif

  # Check if we're skipping rngs
  PHOLD_FIXED = 0
  ifeq ($(PHOLD_FIXED),1)
    CXXFLAGS_PHOLD_FIXED=-DPHOLD_FIXED
    $(info Using fixed rng values with $(CXXFLAGS_PHOLD_FIXED))
    CXXFLAGS+=$(CXXFLAGS_PHOLD_FIXED)
  endif

endif

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
	@echo "SST_REG"
	$(VERB)mkdir -p $(LIBDIR)
	$(VERB)cp $< $(LIBDIR)
	$(VERB)touch $(SSTCONFIG)
	$(VERB)$(SSTREGCMD) phold LIBDIR=$(LIBDIR)
	$(VERB)$(SSTREGCMD) SST_ELEMENT_SOURCE phold=$(SRCDIR)
	$(VERB)$(SSTREGCMD) SST_ELEMENT_TESTS  phold=$(SRCDIR)/tests
	$(VERB)touch $@


.PHONY: all install clean info


clean:
	rm -rf *.o *.d *.so $(LIBDIR)

info:
	@echo "PWD:          $(PWD)"
	@echo "CURDIR:       $(CURDIR)"
	@echo ""
	@echo "SSTCONFIG:    $(SSTCONFIG)"
	@echo "SSTCONFARG:   $(SSTCONFARG)"
	@echo "SSTREGCMD:    $(SSTREGCMD)"
	@echo ""
	@echo "CC:           $(CC) = `which $(CC)`"
	@echo "CXX:          $(CXX) = `which $(CXX)`"
	@echo "CXXFLAGS:     $(CXXFLAGS)"
	@echo "LDFLAGS:      $(LDFLAGS)"
	@echo ""
	@echo "VERBOSE:      $(VERBOSE)"
	@echo "PHOLD_DEBUG:  $(PHOLD_DEBUG)"
	@echo "RNG_DEBUG:    $(RNG_DEBUG)"
	@echo "PHOLD_FIXED:  $(PHOLD_FIXED)"
	@echo ""
	@echo "SRCDIR:       $(SRCDIR)"
	@echo "LIBDIR:       $(LIBDIR)"
	@echo "SRCS:         $(SRCS)"
	@echo "HDRS:         $(HDRS)"
	@echo "ALL:          $(ALL)"
	@echo "OBJS:         $(OBJS)"
	@echo "LIB:          $(LIB)"
	@echo "DEPS:         $(DEPS)"
	@echo "DEPFLAGS:     $(DEPFLAGS)"

