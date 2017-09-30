TOPDIR = ../..
UFW_LOCATION = $(TOPDIR)/ufw
GTEST_LOCATION = $(TOPDIR)/googletest
GTEST_TAP_LOCATION = $(TOPDIR)/gtest-tap-listener
LIBTAP_LOCATION = $(TOPDIR)/libtap

-include $(UFW_LOCATION)/config/ufw-common.mk

CC = gcc
CXX = g++

OPTIMISE = -O0 -g

CFLAGS = $(OPTIMISE) $(C_LANGUAGE_STANDARD)
CFLAGS += $(C_COMPILER_FLAGS_STRICTNESS)
CFLAGS += $(C_COMPILER_FLAG_FATAL_ERRORS)
CFLAGS += -I"$(UFW_LOCATION)"
CFLAGS += -I"$(LIBTAP_LOCATION)"

CXXFLAGS = $(OPTIMISE) $(CXX_LANGUAGE_STANDARD)
CXXFLAGS += $(CXX_COMPILER_FLAGS_STRICTNESS)
CXXFLAGS += $(CXX_COMPILER_FLAG_FATAL_ERRORS)
CXXFLAGS += -I"$(UFW_LOCATION)"
CXXFLAGS += -isystem "$(GTEST_LOCATION)/googletest/include"
CXXFLAGS += -I"$(GTEST_TAP_LOCATION)/src"
CXXFLAGS += -DGTEST_TAP_PRINT_TO_STDOUT

LDFLAGS = -L"$(LIBTAP_LOCATION)" -ltap
LDXXFLAGS = -L"$(GTEST_LOCATION)/build/googlemock/gtest" -lgtest -lgtest_main -lpthread -lgcov

HARNESS = prove --color --normalize --merge --exec "./run"
HFLAGS =
RESULTS = ./results

MKDEP = mkdep
MKDIR = mkdir -p

TESTS = ring-buffer
XXTESTS = RangeValidator TrivialValidator
SOURCES = $(patsubst %,%.c,$(TESTS))
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))
XXSOURCES = $(patsubst %,%.cpp,$(XXTESTS))
XXOBJECTS = $(patsubst %.cpp,%.o,$(XXSOURCES))

run-tests: diagnostics $(OBJECTS) $(XXOBJECTS) $(TESTS) $(XXTESTS) tests.txt
	$(HARNESS) $(HFLAGS) - < tests.txt

test:
	@echo "Running test-suite..."
	$(MAKE) run-tests

test-verbose:
	@echo "Running test-suite in verbose mode..."
	$(MAKE) run-tests HFLAGS='--verbose'

test-verbose-archive:
	@echo "Running test-suite in verbose mode w/ archiving enabled..."
	$(MKDIR) "$(RESULTS)"
	$(MAKE) run-tests HFLAGS='--verbose --archive="$(RESULTS)"'
	(cd "$(RESULTS)" && for i in *; do test "$$i" = meta.yml && continue; mv "$$i" "$${i}.tap"; done;)

tests.txt: Makefile
	:> $@
	for i in $(TESTS); do echo "$$i" >> $@; done
	for i in $(XXTESTS); do echo "$$i" >> $@; done

clean:
	rm -f $(OBJECTS) $(XXOBJECTS) $(TESTS) $(XXTESTS) *.tap tests.txt core *~
	rm -Rf "$(RESULTS)"

distclean: clean
	rm -f .depend *~

depend: diagnostics
	$(MKDEP) $(CXXFLAGS) $(XXSOURCES)
	$(MKDEP) $(CFLAGS) $(SOURCES)

diagnostics::
	@if ! test -f .depend; then printf 'Dependency file missing. You may want to run "make depend".\n'; fi

.suffixes: .c .o

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TESTS): %: %.o $(XXOBJECTS)
	$(CC) $(CFLAGS) -static -o $@ $< $(LDFLAGS)

$(XXTESTS): %: %.o $(XXOBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDXXFLAGS)

-include .depend
.PHONY: clean depend run-tests test test-verbose test-verbose-archive
