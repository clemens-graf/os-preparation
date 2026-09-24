ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := fork_basics exec_wait zombie pipe_demo stdio_trap signals

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run: all
	./fork_basics
	@echo; ./exec_wait
	@echo; ./zombie
	@echo; ./pipe_demo
	@echo; echo "--- stdio_trap to a terminal/pipe/pipe+fflush:"; ./stdio_trap; ./stdio_trap | cat; ./stdio_trap fixed | cat
	@echo; echo "(run ./signals yourself - it is interactive)"

clean:
	rm -f $(PROGS)

.PHONY: all run clean
