ROOT := ../..
include $(ROOT)/common/common.mk

WRAP := -Wl,--wrap=sched_yield

test:
	@mkdir -p build
	$(CC) $(CFLAGS) $(TSAN) -DTHREADS=4 -DITER=2000 -I. tests/test_locks.c $(IMPL)/locks.c -o build/test_locks_tsan $(WRAP) $(LDLIBS)
	$(CC) $(CFLAGS) -O2 -I. tests/test_locks.c $(IMPL)/locks.c -o build/test_locks_fast $(WRAP) $(LDLIBS)
	@echo "=== optimised, full speed ==="
	@$(RUN) build/test_locks_fast; a=$$?; \
	echo "=== ThreadSanitizer ==="; \
	$(RUN_TSAN) build/test_locks_tsan; b=$$?; exit $$((a || b))

clean:
	rm -rf build

.PHONY: test clean
