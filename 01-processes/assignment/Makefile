ROOT := ../..
include $(ROOT)/common/common.mk

# `make` builds ./minish to play with by hand; `make test` runs the test cases.
minish: FORCE
	$(CC) $(CFLAGS) $(ASAN) $(IMPL)/minish.c -o minish $(LDLIBS)

test:
	@mkdir -p build
	$(CC) $(CFLAGS) $(ASAN) $(IMPL)/minish.c -o build/minish $(LDLIBS)
	$(CC) $(CFLAGS) tests/helper.c -o build/helper
	@tests/run_tests.sh build/minish

clean:
	rm -rf build minish

FORCE:
.PHONY: test clean FORCE
