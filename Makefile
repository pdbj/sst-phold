# -*- Mode:make  -*-
#
# Copyright (c) 2021 Lawrence Livermore National Laboratory
# All rights reserved.
#
# Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>

RECURSIVE_TARGETS = all debug opt install clean info

# Make the build itself verbose
VERBOSE = 0
ifeq ($(VERBOSE),1)
  VERB=
else
  VERB=@
  MAKEFLAGS += --no-print-directory
endif

.PHONY: all help debug opt install valgrind sst clean info doc

# Parallel build and install is broken :(
.NOTPARALLEL:

all: Makefile
#	@echo "Making all in src"
	@$(MAKE) -C src all && echo "Make all done" || exit 1

help:
	@echo "Usage: make [target]"
	@echo 
	@echo "Possible targets:"
	@echo 
	@echo "  all [default]  Rebuild anything changed, optimized"
	@echo "  debug          Clean and build everything with debug flags"
	@echo "  opt            Clean and build everything, with default (optimized) flags"
	@echo "  install        Create SST config file"
	@echo "  clean          Remove all built files"
	@echo "  info           Show Makefile variables"

debug: 
	@echo "Cleaning and remaking with PHOLD_DEBUG"
	@$(MAKE) clean
	@$(MAKE) PHOLD_DEBUG=1 RNG_DEBUG=1 all

opt:
	@echo "Cleaning and remaking optimized"
	@$(MAKE) clean all

install:
	@$(MAKE) -C src install

valgrind:
	@$(MAKE) -C src valgrind

clean:
#	@echo "Making clean in src"
	@$(MAKE) -C src clean && echo "Make clean done" || exit 1

info:
#	@echo "Making info in src"
	@$(MAKE) -C src info && echo "Make info done" || exit 1

doc:
	@echo "Doxygen"
	@cd doc && doxygen && cd - 2>&1 /dev/null
	@echo 
	@echo "Doxygen warnings:"
	@cat doc/doxygen.warnings.log | grep -v "'component.h#SST_ELI_REGISTER_COMPONENT' not found" ; true
