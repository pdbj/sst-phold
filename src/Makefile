
SRCDIR = $(PWD)/src
LIBDIR  = $(PWD)/lib

# Check for custom build config resulting from successful install
SSTCONFARG =
SSTCONFIG_PHOLD = $(LIBDIR)/sst-phold.conf
# ifneq (,$(wildcard $(SSTCONFIG_PHOLD)))
#   SSTCONFARG = --lib-path=$(SSTCONFIG_PHOLD)
# endif

SST=$(shell which sst)
SSTCONFIGCMD = sst-config $(SSTCONFARG)
CC=$(shell       $(SSTCONFIGCMD) --CC)
CXX=$(shell      $(SSTCONFIGCMD) --CXX)
CXXFLAGS=$(shell $(SSTCONFIGCMD) --ELEMENT_CXXFLAGS)
LDFLAGS=$(shell  $(SSTCONFIGCMD) --ELEMENT_LDFLAGS)
SSTLIBDIR=$(shell $(SSTCONFIGCMD) SST_ELEMENT_LIBRARY SST_ELEMENT_LIBRARY_LIBDIR)

# Add hard-coded path to help out CLion
#CXXFLAGS += -I$(HOME)/local/include

# Warn if undefined symbols, only works for GCC, not Clang
# LDFLAGS += -Wl,--no-undefined

# Warn all
CXXFLAGS += -Wall -Wpedantic

# Set various build flags
clean_or_install=$(strip \
  $(or $(findstring clean,$(MAKECMDGOALS)), \
       $(findstring install,$(MAKECMDGOALS))))

ifeq ($(clean_or_install),)
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

else
  $(info Building against $(SST))
endif

SRCS = $(wildcard *.cc)
HDRS = $(wildcard *.h)
ALL = $(SRCS) $(HDRS)

OBJS = $(SRCS:%.cc=%.o)

TESTSRC = $(wildcard *test.cc)
TESTOBJ := $(TESTSRC:%.cc=%.o)
TEST = $(TESTSRC:%.cc=%)

LIBOBJS := $(filter-out $(TESTOBJ),$(OBJS))
LIB  = libphold.so

# Make the build itself verbose
VERBOSE = 0
ifeq ($(VERBOSE),1)
  VERB=
else
  VERB=@
  MAKEFLAGS +=--no-print-directory
endif


all: $(LIB) $(TEST)
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

$(LIB): $(LIBOBJS)
	@echo "LD" $@
	$(VERB)$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

SSTREGCMD = sst-register $(SSTCONFARG)
install: $(LIB)
	@echo "SST_REG $(basename $<)" 
	$(VERB)mkdir -p $(LIBDIR)
	$(VERB)cp $< $(LIBDIR)
	$(VERB)mkdir -p $(SSTLIBDIR)
	$(VERB)cp $< $(SSTLIBDIR)
	$(VERB)touch $(SSTCONFIG_PHOLD)
	$(VERB)$(SSTREGCMD) phold LIBDIR=$(LIBDIR)
	$(VERB)$(SSTREGCMD) SST_ELEMENT_SOURCE phold=$(SRCDIR)
	$(VERB)$(SSTREGCMD) SST_ELEMENT_TESTS  phold=$(SRCDIR)/tests


PHOLDTEST=../tests/phold.py
valgrind: install
	@echo VALGRIND sst $(PHOLDTEST)
	$(VERB)valgrind --tool=memcheck --track-origins=yes \
            --show-error-list=yes --trace-children=yes --leak-check=full \
            sst --print-timing-info $(PHOLDTEST) -- -vvvV

LD_TEST = -Wl,-undefined -Wl,dynamic_lookup
$(TEST) : $(TESTOBJ)
	@echo "LD" $@
	$(VERB)$(CXX) $(CXXFLAGS) $(LD_TEST) -o $@ $^

.PHONY: all install clean info valgrind


clean:
	rm -rf *.o *.d *.so $(LIBDIR)

info:
	@echo "PWD:                   $(PWD)"
	@echo "CURDIR:                $(CURDIR)"
	@echo ""
	@echo "SST:                   $(SST)"
	@echo "SSTCONFIG_PHOLD:       $(SSTCONFIG_PHOLD)"
	@echo "SSTCONFARG:            $(SSTCONFARG)"
	@echo "SSTCONFIGCMD:          $(SSTCONFIGCMD)"
	@echo "SSTREGCMD:             $(SSTREGCMD)"
	@echo "SSTLIBDIR:             $(SSTLIBDIR)"
	@echo "SSTELEMLIB:            $(SSTELEMLIB)"
	@echo ""
	@echo "CC:                    $(CC) = `which      $(CC)`"
	@echo "CXX:                   $(CXX) = `which      $(CXX)`"
	@echo "CXXFLAGS:              $(CXXFLAGS)"
	@echo "LDFLAGS:               $(LDFLAGS)"
	@echo ""
	@echo "VERBOSE:               $(VERBOSE)"
	@echo "PHOLD_DEBUG:           $(PHOLD_DEBUG)"
	@echo "CXXFLAGS_PHOLD_DEBUG:  $(CXXFLAGS_PHOLD_DEBUG)"
	@echo "RNG_DEBUG:             $(RNG_DEBUG)"
	@echo "CXXFLAGS_RNG_DEBUG:    $(CXXFLAGS_RNG_DEBUG)"
	@echo "PHOLD_FIXED:           $(PHOLD_FIXED)"
	@echo "CXXFLAGS_PHOLD_FIXED:  $(CXXFLAGS_PHOLD_FIXED)"
	@echo ""
	@echo "SRCDIR:                $(SRCDIR)"
	@echo "LIBDIR:                $(LIBDIR)"
	@echo "SRCS:                  $(SRCS)"
	@echo "HDRS:                  $(HDRS)"
	@echo "ALL:                   $(ALL)"
	@echo "OBJS:                  $(OBJS)"
	@echo "LIB:                   $(LIB)"
	@echo "MAKEFLAGS:             $(MAKEFLAGS)"
	@echo "DEPS:                  $(DEPS)"
	@echo "DEPFLAGS:              $(DEPFLAGS)"
	@echo "PHOLDTEST:             $(PHOLDTEST)"

