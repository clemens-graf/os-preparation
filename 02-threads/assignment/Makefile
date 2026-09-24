ROOT := ../..
include $(ROOT)/common/common.mk

WRAP := -Wl,--wrap=pthread_create -Wl,--wrap=pthread_join

# `make test` = part A (parallel.c) + part B (the three bug programs).
test:
	@$(MAKE) --no-print-directory parallel; a=$$?; $(MAKE) --no-print-directory bugs; b=$$?; exit $$((a || b))

parallel:
	@mkdir -p build
	$(CC) $(CFLAGS) $(TSAN) -I. tests/test_parallel.c $(IMPL)/parallel.c -o build/test_parallel_tsan $(WRAP) $(LDLIBS)
	$(CC) $(CFLAGS) $(ASAN) -I. tests/test_parallel.c $(IMPL)/parallel.c -o build/test_parallel_asan $(WRAP) $(LDLIBS)
	@echo "=== part A under ThreadSanitizer ==="
	@$(RUN_TSAN) build/test_parallel_tsan
	@echo "=== part A under AddressSanitizer ==="
	@$(RUN) build/test_parallel_asan

bugs:
	@mkdir -p build
	$(CC) $(CFLAGS) $(TSAN) $(IMPL)/bugs/bug1_loop_index.c -o build/bug1 $(LDLIBS)
	$(CC) $(CFLAGS) $(ASAN) $(IMPL)/bugs/bug2_dead_arguments.c -o build/bug2 $(LDLIBS)
	$(CC) $(CFLAGS) $(TSAN) $(IMPL)/bugs/bug3_no_join.c -o build/bug3 $(LDLIBS)
	@tests/check_bugs.sh $(ROOT)

clean:
	rm -rf build

.PHONY: test parallel bugs clean
