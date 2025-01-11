# CMake Support {#pagecmake}

## Build System {#cmakebuildsys}

CMake is a meta-buildsystem (meaning that a real buildsystem is generated from
a CMake specification), mostly for C and C++ codebases (there is support for
other languages as well, but that is less commonly used). For better or worse,
`ufw` uses CMake as its buildsystem. Building the library itself is very
simple, as is demonstrated in the @ref simplebuild section.

In a bigger context, the library is intended to be build from source with other
parts of a system. In such scenarios, you normally use `add_subdirectory()` to
include a subtree in a larger build. However for some features, `ufw` needs to
do some additional setup. For most control (which is needed for some features
like Zephyr integration), it is advisable to delay enabling languges in CMake
after some initial setup is done. This is done by handing the `NONE` keyword to
`project()`. In order to do the additional setup automatically, a combination
of the `ufw_toplevel()` and `setup_ufw()` functions from the `SetupUFW` CMake
module should be used in place of `add_subdirectory()`. In order to be able to
use the CMake modules shipped by `ufw` it is necessary to add a directory to
CMake's module load path. If we assume `ufw` is located in `ufw` inside of your
toplevel project directory, common boilerplate to set up `ufw`'s build system
looks like this:

```cmake
# This version is the minimum requirement for Zephyr at the time of writing.
cmake_minimum_required(VERSION 3.20.0)

# Here, "fantastic-firmware" is the name of the project, and should be
# adjusted accordingly. The used of "NONE" delays language initialisation
# until enable_language() is used.
project(fantastic-firmware NONE)

# Extend CMake's load-path and load the SetupUFW utility module.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/ufw/cmake/modules)
include(SetupUFW)

# This is very early setup, that initialises some variables and performs
# low-level system integrations like launching Zephyr's build system, if
# the build uses Zephyr, that is.
ufw_toplevel(
  ROOT      ${CMAKE_SOURCE_DIR}/ufw
  ARTIFACTS ${CMAKE_BINARY_DIR}/artifacts)

# Now enable the languages we need. For C (and to a lesser degree C++), it
# is advisable to also perform ufw's toolchain-initialisation.
enable_language(C)
include(InitialiseToolchain)
initialise_toolchain()

# Finally add ufw to the build system. This means, that the C library of
# ufw will be built and its targets will be available beyond this point.
setup_ufw(libraries/ufw)

# Now there are options, in simple builds can just use CMake to specify
# their requirements here. More complex builds may use mmh's system build
# facilities. With that, it is useful to call this function to dispatch
# to the right build-type.
ufw_subtree_build()
```

As mentioned before, builds for systems that use `ufw` are usually driven by
`mmh`. This is technically not a requirement, but it greatly decreases the
difficulty of calling `cmake`, since `mmh` automatically integrates correctly
with the requirements of `ufw`'s CMake modules. For instance, with Zephyr
builds it automatically sets up variables like:

- `UFW_ZEPHYR_KERNEL`: Location of the Zephyr RTOS sources to be used in build.
- `UFW_ZEPHYR_APPLICATION`: Location of the application source build.

It is possible to do all of this setup without `mmh`. But it is recommended to
use it, if only for convenience. Also note, that depending on the concrete
scenario, it is possible to strip down this boilerplate a lot. For more
information about `mmh` see @ref develmmh.

## CMake Modules {#cmakemodules}

### Core Modules

#### SetupTargetCPU.cmake

#### SetupUFW.cmake

#### BuildArtifacts.cmake

#### HardwareAbstraction.cmake

#### UFWTest.cmake

#### UFWTools.cmake

### Toolchain Features

#### InitialiseToolchain.cmake

#### ToolchainFeatures.cmake

#### UFWCompiler.cmake

#### GNUAttributes.cmake

#### GNUBuiltins.cmake

#### GNUCompilerWarnings.cmake

#### TICompilerWarnings.cmake

#### XilinxISEToolchain.cmake

### Third-Party Support

#### AddNanoPB.cmake

#### ArmCmsis.cmake

#### FreeRTOS.cmake

#### Libtap.cmake

#### Newlib.cmake

#### STCmsis.cmake

#### STM32HAL.cmake

#### STM32HAL_<TARGET>.cmake

### External Tool Support

#### FakeTimeSupport.cmake

#### GenerateGraphics.cmake

#### GitIntegration.cmake {#cmakegitint}

#### pandocology.cmake


## CMake Toolchain Files {#cmaketoolchains}

`mmh` uses CMake's cross-compilation features to build projects (unless they
are Zephyr projects, which use Zephyr's flavour of CMake). Toolchain files need
to be handed into the system very early via the `CMAKE_TOOLCHAIN_FILE`
variable. This in commonly done directly on the CMake configuration command
line.

The cmake-toolchains-files maintained by `ufw` try to follow a common usage
pattern. In terms of CMake these are used by providing the file to the system
at configure time:

```
% cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/ufw/cmake/toolchains/gnu.cmake …
```

It it possible to customise the toolchain's installation directory by setting
an environment variable that matches the following pattern:

```
TOOLCHAIN_ROOT_<UPCASE_UNDERSCORE_TOOLCHAIN_NAME>
```

…where `<UPCASE_UNDERSCORE_TOOLCHAIN_NAME>` for `ti-c2000` would be `TI_C2000`.

Example:

```
% export TOOLCHAIN_ROOT_TI_C2000=/opt/ti/ti-cgt-c2000_20.0.0.LTS
% cmake …
```

Additionally, the names of the C and C++ compiler executables are modifiable by
similarly named CMake variables (not environment variables):

```
TOOLCHAIN_CC_<UPCASE_UNDERSCORE_TOOLCHAIN_NAME>
TOOLCHAIN_CXX_<UPCASE_UNDERSCORE_TOOLCHAIN_NAME>
```

This may be useful with toolchains that allow the installation of different
versions of the same toolchain in the same execution environment, such as gcc
and clang on debian installations.


## Build System Compatibility {#cmakecompat}

Zephyr uses CMake a little different from most other projects using CMake. In
particular the way that toolchains and their parameters are handled are very
different. `ufw` (in conjunction with `mmh`) has support for mediating between
these two worlds. See @ref zephyrsupport for details.
