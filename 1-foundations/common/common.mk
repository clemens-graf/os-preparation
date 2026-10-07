# Shared build settings, included by every module Makefile:
#
#   ROOT := ../..
#   include $(ROOT)/common/common.mk

CC      := gcc
WARN    := -Wall -Wextra -Wshadow -Wno-unused-parameter \
           -Werror=implicit-function-declaration -Werror=return-type \
           -Werror=incompatible-pointer-types -Werror=int-conversion
CFLAGS  := -std=gnu17 -D_GNU_SOURCE $(WARN) -g -O1 -pthread -I$(ROOT)/common
LDLIBS  := -pthread

# AddressSanitizer: out-of-bounds, use-after-free, leaks.  UBSan: undefined behaviour.
ASAN    := -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined
# ThreadSanitizer: data races, lock-order inversions.
TSAN    := -fsanitize=thread
UBSAN   := -fsanitize=undefined -fno-sanitize-recover=undefined

RUN     := $(ROOT)/common/run.sh
RUN_TSAN := $(ROOT)/common/run.sh --tsan

export ASAN_OPTIONS  := detect_stack_use_after_return=1:detect_leaks=1
export TSAN_OPTIONS  := halt_on_error=0:second_deadlock_stack=1:report_signal_unsafe=0
export UBSAN_OPTIONS := print_stacktrace=1

# `make test IMPL=<dir>` builds the tests against the files in <dir> instead of
# your own ones; used to check the reference solutions.
IMPL    ?= .
