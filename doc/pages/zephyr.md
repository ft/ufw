# Zephyr RTOS Support {#zephyrsupport}

TBD.

## Zephyr Build Type Compatibility

Most CMake based build systems use the `CMAKE_BUILD_TYPE` variable to determine
some basic build settings. Common build types are:

- `debug`
- `release`
- `minsizerel`
- `relwithdebinfo`

Zephyr however configures its toolchain via Kconfig variables. `ufw` ships a
Kconfig snippets with appropriate settings for these four common build
configurations. When an `mmh` process builds a Zephyr application, it looks at
`CMAKE_BUILD_TYPE`, and injects the correct snippet file into the build process
automatically.
