ROOT := ../..
include $(ROOT)/common/common.mk

PROGS := pingpong generator switch_cost

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) -O2 $< -o $@ $(LDLIBS)

run: all
	./pingpong
	@echo; ./generator
	@echo; ./switch_cost

clean:
	rm -f $(PROGS)

.PHONY: all run clean
