ROOT := ../..
include $(ROOT)/common/common.mk

INC := -I$(IMPL) -I.

test:
	@mkdir -p build
	$(CC) $(CFLAGS) $(ASAN) $(INC) tests/test_sched.c $(IMPL)/schedsim.c -o build/sched_asan $(LDLIBS) -lm
	@echo "--- sched: AddressSanitizer"; $(RUN) build/sched_asan

clean:
	rm -rf build

.PHONY: test clean
