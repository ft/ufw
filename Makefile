-include config/ufw-common.mk

TOOLCHAIN = arm-none-eabi
CC = $(TOOLCHAIN)-gcc
CXX = $(TOOLCHAIN)-g++

CFLAGS = $(CFLAGS_STRICTNESS) $(CFLAGS_FATAL_WARNINGS)
CFLAGS += -O0 -g -I.

# Seems like a number of embedded vendors don't support c11 yet.
CSTD = c99
CXXSTD = c++11

PERL = perl
COMPILER_TEST = ./config/compiler-test
CT_HEADER = ./config/ufw-config.h
CT_HEADER_PP = ./config/ufw-config-pp.h
CT_MAKE = ./config/ufw-common.mk
CTFLAGS = --c-compiler $(CC) --prefer-c-standard $(CSTD)
CTFLAGS += --cxx-compiler $(CXX) --prefer-cxx-standard $(CXXSTD)
CTFLAGS += --output-c-header "$(CT_HEADER)"
CTFLAGS += --output-cxx-header "$(CT_HEADER_PP)"
CTFLAGS += --output-makefile "$(CT_MAKE)"

all:

config:
	@if ! test -e "$(CT_HEADER)" || \
	    ! test -e "$(CT_HEADER_PP)" || \
	    ! test -e "$(CT_MAKE)"; then \
	    printf 'Running compiler tests...\n'; \
	    $(PERL) $(COMPILER_TEST) $(CTFLAGS); \
	else \
	    true; \
	fi

clean-config:
	rm -f config/ufw-*.*
	rm -f config/*~
	rm -f config/"#"*"#"

clean: clean-config
	$(MAKE) -C test clean
	rm -f *~ "#"*"#"

.PHONY: all clean clean-config config
