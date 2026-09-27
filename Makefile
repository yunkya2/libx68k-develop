all:
	$(MAKE) -C libtsr all
	$(MAKE) -C sample all

check-api-calls:
	$(MAKE) -C sample check-api-calls

clean:
	$(MAKE) -C sample clean
	$(MAKE) -C libtsr clean

.PHONY: all check-api-calls clean
