ROOT := ../..
include $(ROOT)/common/common.mk

INC := -I$(IMPL) -I.

test:
	@mkdir -p build; fail=0; \
	for part in sem rw barber; do \
	  echo "=================== $$part ==================="; \
	  $(CC) $(CFLAGS) $(TSAN) $(INC) tests/test_$$part.c $(IMPL)/sync.c -o build/$${part}_tsan $(LDLIBS) || { fail=1; continue; }; \
	  $(CC) $(CFLAGS) -O2 $(INC) tests/test_$$part.c $(IMPL)/sync.c -o build/$${part}_fast $(LDLIBS) || { fail=1; continue; }; \
	  echo "--- $$part: full speed"; $(RUN) build/$${part}_fast || fail=1; \
	  echo "--- $$part: ThreadSanitizer"; $(RUN_TSAN) build/$${part}_tsan || fail=1; \
	done; \
	if [ $$fail = 0 ]; then echo "==== all parts pass"; else echo "==== some parts still fail"; fi; exit $$fail

test-sem test-rw test-barber: test-%:
	@mkdir -p build
	$(CC) $(CFLAGS) $(TSAN) $(INC) tests/test_$*.c $(IMPL)/sync.c -o build/$*_tsan $(LDLIBS)
	$(CC) $(CFLAGS) -O2 $(INC) tests/test_$*.c $(IMPL)/sync.c -o build/$*_fast $(LDLIBS)
	@$(RUN) build/$*_fast; a=$$?; $(RUN_TSAN) build/$*_tsan; b=$$?; exit $$((a || b))

clean:
	rm -rf build

.PHONY: test test-sem test-rw test-barber clean
