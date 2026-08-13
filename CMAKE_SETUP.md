Author: Cedric Caparas
Date: August 13, 2026

# Getting the SCU repo onto the CMake build

We moved the SCU off the STM32CubeIDE managed build and onto CMake. This is what
you need to do to get your copy working again.

Short version: install STM32CubeCLT, pull, re-import the project in CubeIDE, add
one PATH setting. About 10 minutes.

## What you need to install

**STM32CubeCLT**, go to teams>Software System> Software Resources> Important Software

There is no application to launch, it is just a bundle of command line tools. It
gives you `arm-none-eabi-gcc`, CMake, Ninja, and OpenOCD.

Open a **new** terminal after installing and check:

```bash
arm-none-eabi-gcc --version && cmake --version && ninja --version
```

All three should answer. If they do not, add these to your PATH manually
(adjust the version number to whatever you installed):

```
C:\ST\STM32CubeCLT_1.22.0\GNU-tools-for-STM32\bin
C:\ST\STM32CubeCLT_1.22.0\CMake\bin
C:\ST\STM32CubeCLT_1.22.0\Ninja\bin
```

## Getting your repo up to date

```bash
git pull
```

That is it for the files. The submodule pointer did not change, so you do not
need to touch `Firmware-Library`.

If you never initialized submodules on this machine:

```bash
git submodule update --init --recursive
```

You can delete your old `Debug/` folder whenever you want. It is the old Eclipse
build output and nothing uses it now. It is gitignored so it will not show up in
git status either way. Maybe hang onto it until you have flashed the new build
once and confirmed the car still works.

## Building from the command line

Two configs, that is all there is:

```bash
cmake --preset Debug && cmake --build build/Debug
```

```bash
cmake --preset Release && cmake --build build/Release
```

Output lands in `build/Debug/` or `build/Release/`. You get `SCU2.0.elf`,
`.hex`, `.bin` and `.map`.

The first command configures, the second one builds. You only need to configure
again if `CMakeLists.txt` changes. Day to day just run the build command.

## Building from CubeIDE

The project changed from a managed build project to a CMake project, so your
workspace has stale info about it. You need to re-import it once.

**1. Remove the old project from your workspace**

Right click the project, Delete, and **uncheck "delete contents on disk"**. This
only removes it from the workspace, your files stay put.

**2. Re-import it**

Right click in empty space in the Project Explorer, then pick
**"Create a STM32 CMake project"** then select 'existing CMake sources'. Point it at
your repo folder. Name it `SCU2.0`.

**3. Set up the two build configurations**

Properties > C/C++ Build. Use "Manage Configurations..." to get two configs
named `Debug` and `Release`.

For each one, on the **CMake Settings** tab:

| Field | Debug | Release |
|---|---|---|
| Build directory | `build/Debug` | `build/Release` |
| Generator | `Unix Makefiles` (leave it) | `Unix Makefiles` (leave it) |
| Other options | see below | see below |

Debug:
```
-G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Release:
```
-G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

The Generator dropdown only offers Unix Makefiles because CubeIDE hardcodes it.
Leave it alone, the `-G Ninja` in Other options overrides it. CMake takes the
last `-G` it sees.

On the **Behavior** tab, "Use custom build arguments", set Build arguments to
`-C build/Debug` (or `-C build/Release`). Delete `VERBOSE=1` and `-j` if they
are there.

On the **Builder Settings** tab, change the build command from `make` to
`ninja`.

**4. Fix the compiler PATH (do not skip this)**

Properties > C/C++ Build > **Environment**. Select `PATH`, hit Edit, and put
this at the **front** of the value, keeping everything already there after it:

```
C:\ST\STM32CubeCLT_1.22.0\GNU-tools-for-STM32\bin;C:\ST\STM32CubeCLT_1.22.0\Ninja\bin;C:\ST\STM32CubeCLT_1.22.0\CMake\bin;
```

Do it for both configurations.

This one matters. CubeIDE ships its own older GCC (12.3) and puts it first on
PATH, so without this your builds use a different compiler than everyone else's
and produce a different binary. This setting lives in `.settings/`, which is
gitignored, so it cannot be committed for you. Everyone has to do it once.

## Debugging and flashing

Debugging from CubeIDE works like it always did. Make a normal STM32 C/C++
Application debug config pointed at `build/Debug/SCU2.0.elf`. Breakpoints,
registers, RTOS task view, all the same.

From the command line:

```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/SCU2.0.hex -rst
```

## Debug vs Release

| | Debug | Release |
|---|---|---|
| Optimization | `-O0 -g3` | `-Os` |
| Flash used | ~82KB | ~59KB |
| Debug symbols | yes | no |

Right now they behave identically, the only difference is optimization and debug
info. Serial logging is still on in both. Stripping logs out of Release is a
separate change we have not done yet.

Ideally:
Use Debug for bench work, Release for the car. Test Release separately before it
goes in the car, passing on Debug does not mean Release is fine. Optimization
changes timing and stack usage, which can surface races that `-O0` was hiding.

## Things that will confuse you

**Do not delete `startup_stm32f446xx.s` in the repo root.** It looks like junk
sitting next to `Core/`, but CubeMX generates it there and the build references
it directly. It is the vector table and reset handler, nothing boots without it.

**Do not edit `cmake/stm32cubemx/CMakeLists.txt`.** CubeMX overwrites it every
time you Generate Code. Put your changes in the root `CMakeLists.txt`, which
CubeMX leaves alone.

**If you add a new .c file, add it to the root `CMakeLists.txt`.** CubeMX only
knows about files it generated. It has no idea our sensor drivers or the
Firmware-Library submodule exist, they are listed by hand.

**If a sensor suddenly reads zero, check whether its file is actually in the
build.** HAL declares its callbacks as weak with empty bodies, so if the file
defining a callback is missing you get no build error at all. The empty version
just quietly wins and the sensor reads zero forever.

## If something breaks

Blow away the build folder and reconfigure, it is disposable:

```bash
rm -rf build/Debug && cmake --preset Debug && cmake --build build/Debug
```

If the build cannot find `can_utils.h`, your submodule is empty:

```bash
git submodule update --init --recursive
```

If CubeIDE builds but the CLI does not (or the other way around), it is almost
always the PATH thing in step 4.

Otherwise ask Cedric.
