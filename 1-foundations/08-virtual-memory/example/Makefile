ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := demand_paging cow_fork user_fault

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

# user_fault ends with a deliberate NULL dereference, hence the "-".
run: all
	./demand_paging
	@echo; ./cow_fork
	@echo
	-./user_fault

clean:
	rm -f $(PROGS)

.PHONY: all run clean
