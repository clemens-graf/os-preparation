ROOT := ../..
include $(ROOT)/common/common.mk

INC := -I$(IMPL) -I.

test:
	@fail=0; for part in pagetable fault replace; do \
	  echo "=================== $$part ==================="; \
	  $(MAKE) --no-print-directory test-$$part || fail=1; \
	done; \
	if [ $$fail = 0 ]; then echo "==== all parts pass"; else echo "==== some parts still fail"; fi; exit $$fail

# Parts A and B: your vm.c on the simulated machine.
test-pagetable test-fault: test-%:
	@mkdir -p build
	$(CC) $(CFLAGS) $(ASAN) $(INC) tests/test_$*.c $(IMPL)/vm.c machine.c -o build/$*_asan $(LDLIBS)
	$(CC) $(CFLAGS) -O2 $(INC) tests/test_$*.c $(IMPL)/vm.c machine.c -o build/$*_fast $(LDLIBS)
	@echo "--- $*: AddressSanitizer"; $(RUN) build/$*_asan; a=$$?; \
	echo "--- $*: full speed"; $(RUN) build/$*_fast; b=$$?; exit $$((a || b))

test-replace:
	@mkdir -p build
	$(CC) $(CFLAGS) $(ASAN) $(INC) tests/test_replace.c $(IMPL)/replace.c -o build/replace_asan $(LDLIBS)
	@echo "--- replace: AddressSanitizer"; $(RUN) build/replace_asan

clean:
	rm -rf build

.PHONY: test test-pagetable test-fault test-replace clean
