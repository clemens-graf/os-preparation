ROOT := ../..
include $(ROOT)/common/common.mk

INC := -I$(IMPL) -I.

# No AddressSanitizer or ThreadSanitizer here: both are confused by stack
# switching with swapcontext. UndefinedBehaviorSanitizer is fine, and the
# optimised build catches missing `volatile`s and similar surprises.
test:
	@fail=0; for part in threads sync preempt; do \
	  echo "=================== $$part ==================="; \
	  $(MAKE) --no-print-directory test-$$part || fail=1; \
	done; \
	if [ $$fail = 0 ]; then echo "==== all parts pass"; else echo "==== some parts still fail"; fi; exit $$fail

test-threads test-sync test-preempt: test-%:
	@mkdir -p build
	$(CC) $(CFLAGS) $(UBSAN) $(INC) tests/test_$*.c $(IMPL)/uthread.c -o build/$*_ubsan $(LDLIBS)
	$(CC) $(CFLAGS) -O2 $(INC) tests/test_$*.c $(IMPL)/uthread.c -o build/$*_fast $(LDLIBS)
	@echo "--- $*: UBSan"; $(RUN) build/$*_ubsan; a=$$?; \
	echo "--- $*: full speed"; $(RUN) build/$*_fast; b=$$?; exit $$((a || b))

clean:
	rm -rf build

.PHONY: test test-threads test-sync test-preempt clean
