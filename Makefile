all:
	$(MAKE) -C libx68k all
	$(MAKE) -C libtsr all
	$(MAKE) -C sample all

check-api-calls:
	$(MAKE) -C libx68k all
	$(MAKE) -C sample check-api-calls

clean:
	$(MAKE) -C sample clean
	$(MAKE) -C libtsr clean
	$(MAKE) -C libx68k clean

.PHONY: all check-api-calls clean
