CC = gcc
CXX = g++
MKDEP = mkdep
MKDIR = mkdir -p
-include $(UFW_LOCATION)/config/ufw-common.mk
CFLAGS = $(CFLAGS_STRICTNESS) $(CFLAGS_FATAL_WARNINGS)
CFLAGS += -O0 -g -I. -I.. -Ilibtap -I"$(UFW_LOCATION)"
CXXFLAGS = $(CFLAGS) -Igoogletest/include -Igtest-tap-listener/src
CXXFLAGS += -DGTEST_TAP_PRINT_TO_STDOUT
LDXXFLAGS = -Lgoogletest/googletest -lgtest -lgtest_main -lpthread -lgcov

HARNESS = prove --color --normalize --merge --exec "./run"
HFLAGS =
RESULTS = ./results

TESTS = RangeValidator TrivialValidator
SOURCES = $(patsubst %,%.cpp,$(TESTS))
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

run-tests: diagnostics $(OBJECTS) $(TESTS) tests.txt
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

clean:
	rm -f $(OBJECTS) $(TESTS) *.tap tests.txt core
	rm -Rf "$(RESULTS)"

distclean: clean
	rm -f .depend *~

depend: diagnostics
	$(MKDEP) $(CFLAGS) $(SOURCES)

diagnostics::
	@if ! test -f .depend; then printf 'Dependency file missing. You may want to run "make depend".\n'; fi
	@if ! test -d "$(UFW_LOCATION)"; then printf 'Please define UFW_LOCATION.\n'; exit 1; fi

.suffixes: .c .o

.c.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TESTS): %: %.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDXXFLAGS)

-include .depend
.PHONY: clean depend run-tests test test-verbose test-verbose-archive
