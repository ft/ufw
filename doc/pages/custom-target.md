# Custom Target Support {#ufwcustomtargets}

In embedded development, it is desirable to be able to run cross-compiled code
without the need for specialised hardware. The key component to enable this
sort of process is the excellent [QEMU](https://www.qemu.org), which is a
machine emulator.

The `UFWTest.cmake` module has support for running cross-compiled test suites
through `qemu` and collect any results this yields. This show-cases one of the
strengths of the @ref ufwtap (TAP): A harness that understands the protocol can
interpret result from any and all sources, even remote machines or emulated
execution. An example of TAP output is:

```
1..4
ok 1 - One plus one is two
ok 2 - Zero times four is zero
ok 3 - -3 is negative
ok 4 - 13 is positive
```

Generating this is quite simple, as demonstrated by the @ref ufwtapc
implementation, which is extremely small and highly portable. The portability
property is important, because it makes it possible to express test-suites even
with minimal toolchains, like the bare-metal arm-cortex toolchain (even other
small testing libraries like `libtap` do not trivially support this).

The custom target support in `ufw` allows producing output like this for cross
compiled binaries. It is contained in the `target` subdirectory, with
`target/MYTARGET/bin/run` being the runner for the target architecture. For
example, to run a
[Cortex-M3](https://www.arm.com/products/silicon-ip-cpu/cortex-m/cortex-m3)
binary:

```
% file focus/code-under-test/test/t-hexdump
focus/code-under-test/test/t-hexdump: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), statically linked, with debug_info, not stripped

% ./target/cortex-m3/bin/run focus/code-under-test/test/t-hexdump
1..9
ok 1 - 0 == hexdump(&hd, memory, 32u, 0x1000u)
ok 2 - Line 0 matches expectation
# 00001000  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
ok 3 - Line 1 matches expectation
# 00001010  00 00 00 00 00 00 00 42  00 00 00 00 00 00 00 00  |.......B........|
ok 4 - Final line has a line-feed at the end
ok 5 - Found as many lines as expected: 2
ok 6 - 0 == hexdump(&hd, memory + 20u, 5u, 0x2000u)
ok 7 - Line 0 matches expectation
# 00002000  00 00 00 42 00                                    |...B.|
ok 8 - Final line has a line-feed at the end
ok 9 - Found as many lines as expected: 1

```


## Linux-Style Binaries

The easiest way to make this work is to cross compile the test binaries as
Linux binaries for the desired plattform. In that case you need no specialised
startup code or linker tricks.

The support for [MIPS](https://en.wikipedia.org/wiki/MIPS_architecture) does
exactly that. That is why `target/mips/bin/run` is as trivial as it is.

```
% file focus/code-under-test/test/t-hexdump
focus/code-under-test/test/t-hexdump: ELF 32-bit MSB executable, MIPS, MIPS32 rel2 version 1 (SYSV), statically linked, BuildID[sha1]=23d2542be534a7b476b10a2eb8cad7dd19285b78, for GNU/Linux 3.2.0, with debug_info, not stripped

% ./target/mips/bin/run focus/code-under-test/test/t-hexdump
1..9
ok 1 - 0 == hexdump(&hd, memory, 32u, 0x1000u)
ok 2 - Line 0 matches expectation
# 00001000  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
ok 3 - Line 1 matches expectation
# 00001010  00 00 00 00 00 00 00 42  00 00 00 00 00 00 00 00  |.......B........|
ok 4 - Final line has a line-feed at the end
ok 5 - Found as many lines as expected: 2
ok 6 - 0 == hexdump(&hd, memory + 20u, 5u, 0x2000u)
ok 7 - Line 0 matches expectation
# 00002000  00 00 00 42 00                                    |...B.|
ok 8 - Final line has a line-feed at the end
ok 9 - Found as many lines as expected: 1
```


## Complex Build Targets

As shown in the introduction, `ufw` can run cross-compiled `cortex-m3` code.
With the bare-metal arm-cortex toolchain, however, there no way to build for
Linux's calling conventions. To support execution through QEMU's system
emulator, `ufw` contains custom startup code in
`target/cortex-m3/init/startup.c`, and linker configuration for the `LM3s6965`
evaluation board by Texas Instruments (which QEMU has support for out of the
box) in `target/cortex-m3/ld/arm-cortex-qemu.ld`. When specifying a test
program using `ufw_test_program()`, `ufw` takes care of modifying the program's
build parameters to automatically use these.

Among other things, the startup code enables semihosting and connects stdout to
it. The `run` script then connects the semihosting output to QEMU's stdout, and
it disables automatic reboots of the binary. That is why it looks like the
`cortex-m3` binary runs like a native program. This enables running the
binaries inside a TAP harness just like any other program.
