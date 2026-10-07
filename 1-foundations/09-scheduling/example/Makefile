ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := nice_share wakeup_latency ctxt_switches

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run: all
	./nice_share
	@echo; ./wakeup_latency
	@echo; ./ctxt_switches

clean:
	rm -f $(PROGS)

.PHONY: all run clean
