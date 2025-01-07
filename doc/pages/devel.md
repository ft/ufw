# Developer Documentation {#developerdocs}

TBD.


## MakeMeHappy Module Support {#develmmh}

The `mmh` build program uses some of `ufw`'s CMake modules to perform its job.
In other words, if you would like to use `mmh` for build orchestration, you
must depend on `ufw`. It is not required to link against any of the `ufw`
libraries, but some of the CMake modules are non-optional. On top of this,
`mmh` has support for running modular builds. For this to work, modules need to
specify their features and requirements. The file to perform this specification
is `module.yaml`, which needs to exist in a software module's root directory.
`ufw` is such a module. It is a also a leaf-node in dependency graphs (i.e. it
does not have any additional dependencies). Next to the `ufw` C library, the
module specification lists CMake modules, and toolchains, and extensions.

As such, `ufw` can serve as a foundational piece for modularised software
builds. That is libraries, applications, as well as Zephyr applications.


## Fuzz-Testing for ufw {#develfuzz}

### Introduction

It is possible to  run fuzzers on parts of ufw. The fuzzer  used by the project
is `AFL++`, which needs to be installed as a prerequisite. For details see:

- https://aflplus.plus
- https://github.com/AFLplusplus/AFLplusplus

AFL++ comes with a program called `afl-fuzz`, which can be used to feed
arbitrary data into a program's stdin in order to exercise it. The ufw library
has programs using the library that can be used with this scheme.

The fuzzing integration is implemented in the `fuzz` sub-directory of the
project. The following assumes the user is in this directory. The "run" program
in this directory helps with using `AFL++`.


### Initialisation

```
./run init
```

This compiles  ufw in native mode,  with the compiler being  set to `afl-clang`
into a directory called `build-afl`.  All instrumented example programs will be
located in and be run from this build directory tree.

It is possible to adjust `AFL++` installation specifics using these options:

```
  -A           AFL++ source directory
```

This allows to use `AFL++` from its source directory instead of from a
system-wide installation.

```
  -c COMPILER  Set compiler to COMPILER instead of "afl-clang".
  -r RUNNER    Set fuzzing runner to RUNNER instead of "afl-fuzz".
```

Based on these options, two  files `afl-compiler` and `alf-runner` are created.
The `run` script will use these to invoke the associated functionality.

For more control, you can also ask "init" to do the minimal amount of work:

```
  -m           Only initialise afl-compiler and afl-runner
```

After that the following can be used to setup the `build-afl` tree:

```
./run configure
```

And finally the `build` subcommand can be used to actually build ufw inside of
the `build-afl` sub-directory:

```
./run build
```

This command can also be used to re-run the compilation process in case the
library's code was changed.

When the `-m` option is not used with `init`, the sub-command runs the
`configure` and `build` steps automatically.


### Running Programs

```
./run fuzzer SUB-DIRECTORY
```

Each runnable program is associated with a sub-directory inside of `fuzz`. The
name of these directories matches the name of the example programs in `ufw`'s
`example` sub-directory.

Examples that do not have such a directory are not supported to be fuzzed.

And additional argumants passed to the sub-command after the `SUB-DIRECTORY`
parameter are passed to the afl-runner (`afl-fuzz`) program.


### Cleaning Up

```
./run clean
```

This cleans up build artefacts from other `run` invocations. Use the `-a`
option to `clean` to remove fuzzer outputs as well.


### Full Example

Here is a full example, which assumes that `AFL++` is installed in
`~/src/AFLplusplus`:

```
./run init -A ~/src/AFLplusplus
./run fuzzer ex-regp-parse-frame
```

The final command will run `afl-fuzz` with an instrumented version of
`examples/ex-regp-parse-frame.c` with state from `fuzz/ex-regp-parse-frame`. To
exit the program, press `CTRL-C`.
