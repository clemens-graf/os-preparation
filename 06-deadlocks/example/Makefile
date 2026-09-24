ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := deadlock_demo philosophers inversion

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run: all
	./deadlock_demo
	./philosophers naive
	./philosophers
	./inversion

# ThreadSanitizer finds lock-order inversions even when the deadlock does
# NOT happen in this particular run: it records "A was held while taking B"
# and complains when it later sees "B held while taking A".
tsan: inversion.c
	$(CC) $(CFLAGS) $(TSAN) inversion.c -o inversion_tsan $(LDLIBS)
	-$(RUN_TSAN) ./inversion_tsan 2>&1 | grep -E "WARNING|Cycle|Mutex M|finished" | head -8

clean:
	rm -f $(PROGS) inversion_tsan

.PHONY: all run tsan clean
