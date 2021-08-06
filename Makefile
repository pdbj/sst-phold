
RECURSIVE_TARGETS = all clean debug info

all: Makefile
	@echo "Making all in src"
	@$(MAKE) -C src all && echo "Make all done" || exit 1

debug: 
	@echo "Cleaning and remaking with PHOLD_DEBUG"
	@$(MAKE) PHOLD_DEBUG=1 RNG_DEBUG=1 clean all

clean:
	@echo "Making clean in src"
	@$(MAKE) -C src clean && echo "Make clean done" || exit 1

info:
	@echo "Making info in src"
	@$(MAKE) -C src info && echo "Make info done" || exit 1


