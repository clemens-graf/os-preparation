ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := bounded_buffer lost_wakeup semaphores

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run: all
	./bounded_buffer | tail -8
	@echo; ./lost_wakeup
	@echo; ./semaphores

clean:
	rm -f $(PROGS)

.PHONY: all run clean
