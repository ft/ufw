# Test Anything Protocol Support {#ufwtap}

## Portable C Testing Library {#ufwtapc}

The goal of this library was to implement a reasonable testing API, that can be
implemented extremely portably, so that it can be used with the most curious of
embedded targets. This part of `ufw` is contained in the `ufw-tap` library,
which tests must link to in order to use the library. A minimal example on how
to set this up can be found in @ref mmhtinylib. A more complex test-suite using
it is `ufw`'s test-suite itself. It is located in the `test` subdirectory of
the project.

The API is documented in `ufw/test/tap.h`. The API supports both deterministic
and non-deterministic TAP test suites. Deterministic suites look like this:

```c
#include <stdlib.h>
#include <ufw/test/tap.h>
int
main(void)
{
    plan(123);
    /* Tests go here. "plan" implies "tap_init()". */
    return EXIT_SUCCESS;
}
```

Non-deterministic suites on the other hand look like this:

```c
#include <stdlib.h>
#include <ufw/test/tap.h>
int
main(void)
{
    tap_init();
    /* Tests go here. */
    no_plan();
    return EXIT_SUCCESS;
}
```

## Portable POSIX Shell Testing Library {#ufwtapsh}

For some purposes it is useful to be able to express TAP style test-suites in
POSIX shell scripts. `ufw` has support for this via the `bin/tap.sh` library.
It is fairly featureful, portable, and fully self-contained. Except for POSIX
`sed`, it has no external dependencies.

The main driver is the `tap` function. Deterministic test suites should use the
`plan` function at the beginning of their execution. Non-deterministic
test-suites should call the `noplan` function at the very end of their
execution.

The "tap" function works like this:

```
  tap [OPTION(s)...] <TEST-RUNNER> <TEST-TITLE>
```

The TEST-RUNNER and TEST-TITLE arguments are non-optional. The latter is a
string, that should describe the test that is executed, short and to the point.
The TEST-RUNNER argument is something executable: Often a function, but
external programs and scripts also work. It is called with zero arguments and
its return-code (success or failure) determine the result of the test.

Available options of "tap":

```
  -p <PREREQUISITE-RUNNER>
        This option takes something executable, and if this executions
        indicates failure, it causes the test to be skipped. Successful
        execution causes the test to be carried out.

  -s <SETUP-RUNNER>
        This option takes something executable, and runs it just before
        a test is executed. If a prerequisite runner failed, this option
        is ignored. The runner should return success. Failure will emit
        a warning, and fail the test without running it.

  -t <TEARDOWN-RUNNER>
        This option is similar to "-s", only that the runner gets executed
        just AFTER a test is executed. Returning an error from this runner
        will invalidate the test.

There are also two pre-commands for "tap", that modify a test's behaviour:

  SKIP tap ...
        This marks a test as skipped and sets its result to success. Any
        runner options (-p, -s, and -t) will have no effect and the test
        runner itself is not executed either.

  TODO tap ...
        This marks a test as known-to-fail. The execution of all runners
        is carried out as normal, but TAP harnesses will not fail a suite
        with a failing test that is marked TODO.
```

Example:

```sh
#!/bin/sh

# This variable should be set in your environment.
. "${MICROFRAMEWORK_ROOT}"/bin/tap.sh

plan 1

setup () {
    printf '# I am a setup procedure!\n'
}

teardown () {
    printf '# I am a teardown procedure!\n'
}

prereq () {
    printf '# I am a prerequisite that always succeeds!\n'
}

the_test () {
    [ $(( 1 + 1 )) = 2 ]
}

tap -s setup -t teardown -p prereq the_test 'One plus one equals two'
```

Output:

```
1..1
# I am a prerequisite that always succeeds!
# I am a setup procedure!
# I am a test that always succeeds!
# I am a teardown procedure!
ok 1 - Example test passes
```
