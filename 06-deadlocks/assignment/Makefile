ROOT := ../..
include $(ROOT)/common/common.mk

INC := -I$(IMPL) -I.

# ThreadSanitizer's own lock-order checker is turned off for two parts:
#  - dining: a correct "waiter" solution still takes forks left-then-right,
#    which TSan would flag although the waiter makes the cycle impossible;
#  - lockdep: the tests invert lock orders ON PURPOSE to trigger your checker.
TSAN_ENV_dining   := TSAN_OPTIONS=$(TSAN_OPTIONS):detect_deadlocks=0
TSAN_ENV_accounts :=
TSAN_ENV_lockdep  := TSAN_OPTIONS=$(TSAN_OPTIONS):detect_deadlocks=0

test:
	@fail=0; for part in accounts dining lockdep; do \
	  echo "=================== $$part ==================="; \
	  $(MAKE) --no-print-directory test-$$part || fail=1; \
	done; \
	if [ $$fail = 0 ]; then echo "==== all parts pass"; else echo "==== some parts still fail"; fi; exit $$fail

test-accounts test-dining test-lockdep: test-%:
	@mkdir -p build
	$(CC) $(CFLAGS) $(TSAN) $(INC) tests/test_$*.c $(IMPL)/$*.c -o build/$*_tsan $(LDLIBS)
	$(CC) $(CFLAGS) -O2 $(INC) tests/test_$*.c $(IMPL)/$*.c -o build/$*_fast $(LDLIBS)
	@echo "--- $*: full speed"; $(RUN) build/$*_fast; a=$$?; \
	echo "--- $*: ThreadSanitizer"; $(TSAN_ENV_$*) $(RUN_TSAN) build/$*_tsan; b=$$?; exit $$((a || b))

clean:
	rm -rf build

.PHONY: test test-accounts test-dining test-lockdep clean
