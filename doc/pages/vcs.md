# Version Control System Support

`ufw` features some support for information from the [git version control
system](https://git-scm.com). This is powered by a portable POSIX shell library
in `vcs-integration/git.sh`, a couple of utilities in `bin` and CMake
integration via @ref cmakegitint.


## Generating Version Header File

One common desire is to create a C header file at build time, that reflects the
state of the `git` repository of a software system. `ufw`'s
`GitIntegration.cmake` module has a utility function to perform this task.
Example:

```cmake
include(GitIntegration)
generate_version_h(
  TARGET   this-firmwares-version-h
  TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
  OUTPUT   "${CMAKE_CURRENT_BINARY_DIR}/version.h")
```

With this, a larger software build can add a dependency to a new target called
`this-firmwares-version-h`, that will generate the requested file for the
system to use.


## Generating Revision File

Similarly, some build tasks (often documentation builds) need a file that
contains a representation of the `git` repository's state. This utility does
exactly that:

```cmake
include(GitIntegration)
generate_revision_file(
  TARGET this-firmwares-revision-txt
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/revision.txt")
```

Other targets can add a dependency for the `this-firmwares-revision-txt` target
in order to generate the file automatically when it is needed.
