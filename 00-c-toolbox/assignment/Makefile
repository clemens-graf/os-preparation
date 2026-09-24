ROOT := ../..
include $(ROOT)/common/common.mk

test:
	@mkdir -p build
	$(CC) $(CFLAGS) $(ASAN) -I. tests/test_toolbox.c $(IMPL)/toolbox.c -o build/test_toolbox $(LDLIBS)
	@$(RUN) build/test_toolbox

clean:
	rm -rf build

.PHONY: test clean
