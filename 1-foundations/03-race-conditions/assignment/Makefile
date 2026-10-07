ROOT := ../..
include $(ROOT)/common/common.mk

# Each part is tested twice:
#   *_tsan : under ThreadSanitizer, which reports every unsynchronised access
#   *_fast : optimised, no sanitizer - races show up as broken invariants
INC  := -I$(IMPL) -I.
FAST := -O2

test:
	@mkdir -p build; fail=0; \
	for part in bank stats once; do \
	  echo "=================== $$part ==================="; \
	  $(CC) $(CFLAGS) $(TSAN) $(INC) tests/test_$$part.c $(IMPL)/$$part.c -o build/$${part}_tsan $(LDLIBS) || { fail=1; echo ">>> $$part: does not compile"; continue; }; \
	  $(CC) $(CFLAGS) $(FAST) $(INC) tests/test_$$part.c $(IMPL)/$$part.c -o build/$${part}_fast $(LDLIBS) || { fail=1; continue; }; \
	  echo "--- $$part: full speed"; $(RUN) build/$${part}_fast || fail=1; \
	  echo "--- $$part: ThreadSanitizer"; $(RUN_TSAN) build/$${part}_tsan || fail=1; \
	done; \
	if [ $$fail = 0 ]; then echo "==== all parts pass"; else echo "==== some parts still fail"; fi; exit $$fail

test-bank test-stats test-once: test-%:
	@mkdir -p build
	$(CC) $(CFLAGS) $(TSAN) $(INC) tests/test_$*.c $(IMPL)/$*.c -o build/$*_tsan $(LDLIBS)
	$(CC) $(CFLAGS) $(FAST) $(INC) tests/test_$*.c $(IMPL)/$*.c -o build/$*_fast $(LDLIBS)
	@$(RUN) build/$*_fast && $(RUN_TSAN) build/$*_tsan

clean:
	rm -rf build

.PHONY: test test-bank test-stats test-once clean
