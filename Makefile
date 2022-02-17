
RECURSIVE_TARGETS = all debug opt install clean info

# Make the build itself verbose
VERBOSE = 0
ifeq ($(VERBOSE),1)
  VERB=
else
  VERB=@
  MAKEFLAGS += --no-print-directory
endif


all: Makefile
#	@echo "Making all in src"
	@$(MAKE) -C src all && echo "Make all done" || exit 1

debug: 
	@echo "Cleaning and remaking with PHOLD_DEBUG"
	@$(MAKE) PHOLD_DEBUG=1 RNG_DEBUG=1 clean all

opt:
	@echo "Cleaning and remaking optimized"
	@$(MAKE) clean all

install:
	@echo "Installing"
	@$(MAKE) -C src install

clean:
#	@echo "Making clean in src"
	@$(MAKE) -C src clean && echo "Make clean done" || exit 1

info:
#	@echo "Making info in src"
	@$(MAKE) -C src info && echo "Make info done" || exit 1


