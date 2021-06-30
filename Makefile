
RECURSIVE_TARGETS = all-recursive clean-recursive

all: Makefile
	@echo "Making all in src"
	@$(MAKE) -C src all && echo "Make all done" || exit 1

clean:
	@echo "Making clean in src"
	@$(MAKE) -C src clean && echo "Make clean done" || exit 1

info:
	@echo "Making info in src"
	@$(MAKE) -C src info && echo "Make info done" || exit 1


