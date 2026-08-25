all:
	$(MAKE) -C libx68k all
	$(MAKE) -C libtsr all
	$(MAKE) -C sample all

clean:
	$(MAKE) -C sample clean
	$(MAKE) -C libtsr clean
	$(MAKE) -C libx68k clean

.PHONY: all clean
