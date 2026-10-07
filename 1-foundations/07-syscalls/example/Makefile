ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := raw_syscall user_pointers hello freestanding

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) -Wno-unused-result -Wno-nonnull $< -o $@ $(LDLIBS)

# No C library, no startup files: the program brings its own _start.
freestanding: freestanding.c
	$(CC) -std=gnu17 -O1 -g -Wall -ffreestanding -fno-stack-protector -fno-pie \
	      -nostdlib -static -no-pie $< -o $@

run: all
	./raw_syscall
	@echo; ./user_pointers
	@echo; ./freestanding one two; echo "exit status: $$?"

# Every system call a program makes, with arguments and return values.
# (freestanding exits with status argc = 1, which make would call an error: "-")
strace: hello freestanding
	strace ./hello
	@echo; echo "--- only the writes, with stdout going to a pipe (fully buffered):"
	strace -e trace=write ./hello | cat
	@echo; echo "--- the same without libc: execve, the writes, exit_group - nothing else"
	-strace ./freestanding

clean:
	rm -f $(PROGS)

.PHONY: all run strace clean
