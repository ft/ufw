-include config/ufw-common.mk

TOOLCHAIN = arm-none-eabi
CC = $(TOOLCHAIN)-gcc
CFLAGS = $(CFLAGS_STRICTNESS) $(CFLAGS_FATAL_WARNINGS)
CFLAGS += -O0 -g -I.

# Seems like a number of embedded vendors don't support c11 yet.
CSTD = c99

PERL = perl
COMPILER_TEST = ./config/compiler-test
CT_HEADER = ./config/ufw-config.h
CT_MAKE = ./config/ufw-common.mk
CTFLAGS = --compiler $(CC) --prefer-standard $(CSTD)
CTFLAGS += --output-c-header "$(CT_HEADER)"
CTFLAGS += --output-makefile "$(CT_MAKE)"

all:

config:
	@if ! test -e "$(CT_HEADER)" || \
	    ! test -e "$(CT_MAKE)"; then \
	    printf 'Running compiler tests...\n'; \
	    $(PERL) $(COMPILER_TEST) $(CTFLAGS); \
	else \
	    true; \
	fi

clean:
	rm config/ufw-*.*

.PHONY: all clean config
