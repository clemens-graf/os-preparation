ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := lost_update interleave check_then_act three_fixes

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run: all
	./lost_update 1
	./lost_update 2
	./lost_update 8
	@echo; ./interleave
	@echo; ./check_then_act
	@echo; ./three_fixes

# The three instructions behind counter++
asm: lost_update
	objdump -d --no-show-raw-insn -M intel lost_update | awk '/<increment>:/,/ret/'

# Let ThreadSanitizer point at the race (source line + both threads' stacks)
tsan: lost_update.c
	$(CC) $(CFLAGS) $(TSAN) lost_update.c -o lost_update_tsan $(LDLIBS)
	-$(RUN_TSAN) ./lost_update_tsan 2

clean:
	rm -f $(PROGS) lost_update_tsan

.PHONY: all run asm tsan clean
