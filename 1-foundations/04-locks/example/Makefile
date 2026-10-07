ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := peterson spinlock_tas cas

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) -O2 $< -o $@ $(LDLIBS)

run: all
	./peterson
	@echo; ./spinlock_tas
	@echo; ./cas

# The atomic instructions the compiler emits for the lock functions
asm: spinlock_tas cas
	@objdump -d --no-show-raw-insn -M intel spinlock_tas | grep -E "xchg .*PTR|lock " | head -5
	@objdump -d --no-show-raw-insn -M intel cas | grep -E "cmpxchg" | head -5

clean:
	rm -f $(PROGS)

.PHONY: all run asm clean
