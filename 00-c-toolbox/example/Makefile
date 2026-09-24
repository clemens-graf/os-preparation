ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := pointers lifetime callbacks

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $(ASAN) $< -o $@ $(LDLIBS)

run: all
	./pointers
	@echo
	./lifetime
	@echo
	./callbacks

# The three deliberately broken variants of lifetime.c. Each one should make
# AddressSanitizer print a report; read the first lines of it.
bug-stack: lifetime
	-./lifetime stack
bug-uaf: lifetime
	-./lifetime uaf
bug-leak: lifetime
	-./lifetime leak

clean:
	rm -f $(PROGS)

.PHONY: all run clean bug-stack bug-uaf bug-leak
