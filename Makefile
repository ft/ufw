-include config/ufw-common.mk

TOOLCHAIN = arm-none-eabi
CC = $(TOOLCHAIN)-gcc
CFLAGS = $(CFLAGS_STRICTNESS) $(CFLAGS_FATAL_WARNINGS)
CFLAGS += -O0 -g -I.

# Seems like a number of embedded vendors don't support c11 yet.
CSTD = c99

PERL = perl
COMPILER_TEST = ./config/compiler-test
CTFLAGS = --compiler $(CC) --prefer-standard $(CSTD)
CTFLAGS += --output-c-header ./config/ufw-config.h
CTFLAGS += --output-makefile ./config/ufw-common.mk

all:

config:
	$(PERL) $(COMPILER_TEST) $(CTFLAGS)

.PHONY: all config
