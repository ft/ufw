# Overview {#overview}

The library before you is called `micro-framework`; or `μfw` for short; or —
more commonly and to avoid the requirement for characters beyond ASCII — `ufw`.
Its main core consists of a portable C library, implementing various utilities
for embedded work, as well as a set of CMake modules and toolchain files, that
can be used in the embedded realm, and try to solve some of the more unique
issues found in that world. Additionally, `ufw` contains some support and
extensions for the [Zephyr Real Time Operating
System](https://zephyrproject.org). This support lies mostly in smoothing over
some impedance mismatches in which Zephyr uses CMake, compared to most other
projects. The extensions to Zephyr are contained in a separate part of the
library that can optionally be enabled via
[Kconfig](https://docs.zephyrproject.org/latest/build/kconfig/index.html).
`ufw` also provides some integration with the [git version control
system](https://git-scm.com), offering various ways to access information
stored within the system. The library offers support for software tests, using
the [Test Anything Protocol (TAP)](https://testanything.org), via a tiny, very
portable C implementation, and a portable POSIX shell compatible implementation
as well. In addition to this, `ufw` also supports running test-suites for
foreign architectures via [QEMU](https://www.qemu.org), using custom startup
code in some cases (such as for the `cortex-m3` architecture).


## Building the Library {#simplebuild}

For convenient cross-compilation of the library, the use of its companion tool
`mmh` (short for [MakeMeHappy](https://github.com/ft/makemehappy)) is advised.
This is not a strict requirement. The library can be build via CMake alone
completely. For instance, the usual `cmake` build for the host system just
works:

```sh
% mkdir build
% cd build
% cmake ..
% make all test
```

Which is easy enough. For cross-builds, things get a little bit more involved:

```sh
% mkdir build-cm3
% cd build-cm3
% cmake -S .. -B .                         \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=on \
        -DPROJECT_TARGET_CPU=cortex-m3     \
        -DCMAKE_BUILD_TYPE=debug           \
        -DCMAKE_TOOLCHAIN_FILE=$PWD/../cmake/toolchains/gnu-arm-none-eabi.cmake
% make all test
```

`mmh` is a tool that (among other things) helps with building these elaborate
command lines. With it, it is possible to do this:

```sh
% mmh -d build-cm3 prepare
% mmh -d build-cm3 run cmake/cortex-m3/ufw/gnu-arm-none-eabi/debug/make
```

Which achieves the same result as the previous large command. `mmh` comes with
comprehensive completion support for `zsh` (the
[Z-Shell](https://www.zsh.org)), which greatly eases typing out instance names
such as the one used in this example. If used without `zsh`, `mmh` allows for
all the introspection needed, to form these command lines as well. For example,
to find a list of all available build-instances available in a build tree, use:

```sh
% mmh -d build-cm3 list
```

`mmh` was designed to exercise a codebase with many combinations of toolchains,
build-configurations, build-tools, and target architectures. It is very
flexible, is routinely used as part of `ufw`'s development, and has many
features that make it a valuable companion in continuous-integration (CI)
operations.

In normal embedded system development, `ufw` would be used in a larger CMake
based build. It is not strictly required to use `ufw`'s CMake modules to set up
a build system, but it is recommended to do so. `ufw` also integrates well with
Zephyr's build-system, in which the library can be used as a Zephyr-module, as
which it fully integrates with Zephyr's configuration and build flags. And
while it is possible to build `ufw` standalone alongside Zephyr, it is
recommended to use it as a Zephyr-module when building a Zephyr application.


## Library Pieces

The `ufw` library is really a set of multiple libraries:

- `ufw` — This is the core library containing most of its features, all of them
  intended for production purposes, unless marked experimental otherwise.
- `ufw-sx` — This part contains the "Simple S-Expression Parser" feature. It is
  mostly meant for automatic system integration tests. This library uses
  `malloc()`, which makes it unusable for many embedded purposes.
- `ufw-tap` — This part contains the Test Anything Protocol emitting testing
  framework. This is meant for testing purposes. It uses variable argument
  lists.

With `native` targets, the system also builds `-nosan` versions of all these
libraries, which disable compiler sanitizers, whereas the default builds enable
them whenever available.

Note that there is also a dynamic library (`libufw-full.so`), that is build
with `native` targets. This is only built for running ABI/API compatibility
tests. This library is **NOT** intended for normal use, but for this
maintenance task only. Users should stick to the static libraries mentioned
above. The dynamic library is not supported.
