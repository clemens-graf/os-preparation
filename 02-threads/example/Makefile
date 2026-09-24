ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := hello_threads shared_vs_private return_values

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run: all
	./hello_threads
	@echo; ./shared_vs_private
	@echo; ./return_values

clean:
	rm -f $(PROGS)

.PHONY: all run clean
