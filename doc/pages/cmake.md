# CMake Support

## CMake Modules

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

#### GitIntegration.cmake

#### pandocology.cmake


## CMake Toolchain Files

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


## Build System Compatibility

Zephyr uses CMake a little different from most other projects using CMake. In
particular the way that toolchains and their parameters are handled are very
different. `ufw` (in conjunction with `mmh`) has support for mediating between
these two worlds. See @ref zephyrsupport for details.
