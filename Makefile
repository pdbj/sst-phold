
RECURSIVE_TARGETS = all-recursive clean-recursive

all: all-recursive

all-recursive:
	echo "Making all in src"
	$(MAKE) -C src all || exit 1

clean: clean-recursive

clean-recursive:
	echo "Makeing clean in src"
	$(MAKE) -C src clean || exit 1
