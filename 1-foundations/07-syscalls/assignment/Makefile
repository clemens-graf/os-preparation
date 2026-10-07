ROOT := ../..
include $(ROOT)/common/common.mk

INC := -I$(IMPL) -I.

# Part A must not call into glibc. Compiled on its own, without sanitizers
# and stack protector, mini_libc.o may only refer to _GLOBAL_OFFSET_TABLE_
# (provided by the linker for position-independent code, not by glibc).
NOLIBC := -std=gnu17 -D_GNU_SOURCE -O2 -ffreestanding -fno-stack-protector \
          -fno-tree-loop-distribute-patterns

test:
	@fail=0; for part in libc uaccess syscalls; do \
	  echo "=================== $$part ==================="; \
	  $(MAKE) --no-print-directory test-$$part || fail=1; \
	done; \
	if [ $$fail = 0 ]; then echo "==== all parts pass"; else echo "==== some parts still fail"; fi; exit $$fail

test-libc:
	@mkdir -p build
	$(CC) $(NOLIBC) $(INC) -c $(IMPL)/mini_libc.c -o build/mini_libc_nolibc.o
	$(CC) $(CFLAGS) $(ASAN) $(INC) tests/test_libc.c $(IMPL)/mini_libc.c -o build/libc_asan $(LDLIBS)
	$(CC) $(CFLAGS) -O2 $(INC) tests/test_libc.c $(IMPL)/mini_libc.c -o build/libc_fast $(LDLIBS)
	@echo "--- libc: nothing from glibc?"; u=$$(nm -u build/mini_libc_nolibc.o | grep -v _GLOBAL_OFFSET_TABLE_); \
	if [ -n "$$u" ]; then echo ">>> mini_libc.c uses functions from glibc:"; echo "$$u"; f=1; \
	else echo "    ok, no glibc functions used"; f=0; fi; \
	echo "--- libc: AddressSanitizer"; $(RUN) build/libc_asan; a=$$?; \
	echo "--- libc: full speed"; $(RUN) build/libc_fast; b=$$?; exit $$((f || a || b))

test-uaccess test-syscalls: test-%:
	@mkdir -p build
	$(CC) $(CFLAGS) $(ASAN) $(INC) tests/test_$*.c $(IMPL)/kernel.c machine.c -o build/$*_asan $(LDLIBS)
	$(CC) $(CFLAGS) -O2 $(INC) tests/test_$*.c $(IMPL)/kernel.c machine.c -o build/$*_fast $(LDLIBS)
	@echo "--- $*: AddressSanitizer"; $(RUN) build/$*_asan; a=$$?; \
	echo "--- $*: full speed"; $(RUN) build/$*_fast; b=$$?; exit $$((a || b))

clean:
	rm -rf build

.PHONY: test test-libc test-uaccess test-syscalls clean
