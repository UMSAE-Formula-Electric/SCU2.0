Author: Cedric Caparas
Date: August 13, 2026

# SCU build setup

The SCU builds with CMake. CubeMX and CubeIDE work the same as before.

## 1. Install STM32CubeCLT

On Teams. Software System>Software Resources>Important Software
Open a new terminal and check:

```bash
arm-none-eabi-gcc --version && cmake --version && ninja --version
```

If any are missing, add to PATH:

```
C:\ST\STM32CubeCLT_<version>\GNU-tools-for-STM32\bin
C:\ST\STM32CubeCLT_<version>\CMake\bin
C:\ST\STM32CubeCLT_<version>\Ninja\bin
```

## 2. Get the code

```bash
git submodule update --init --recursive
```

## 3. Build from the command line

```bash
cmake --preset Debug && cmake --build build/Debug
```

```bash
cmake --preset Release && cmake --build build/Release
```

Output is in `build/<config>/`. Re-run the `--preset` step only when
`CMakeLists.txt` changes.

## 4. Build from CubeIDE

Re-import the project once:

1. Right click the project, Delete, **uncheck "delete contents on disk"**
2. Right click empty space in Project Explorer, pick **"Create a STM32 CMake
   project using existing CMake sources"**, point it at the repo
3. Properties > C/C++ Build, create configurations named `Debug` and `Release`

Set these per configuration:

| Tab | Setting |
|---|---|
| CMake Settings > Build directory | `build/Debug` or `build/Release` |
| CMake Settings > Other options | `-G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` |
| Behavior > Build arguments | `-C build/Debug` or `-C build/Release` |
| Builder Settings > Build command | `ninja` |
| Environment > PATH | prepend CubeCLT's `GNU-tools-for-STM32\bin`, `Ninja\bin`, `CMake\bin` |

Leave the Generator dropdown alone, `-G Ninja` overrides it.

The PATH step is required. Without it CubeIDE uses its own older GCC and your
binary differs from everyone else's. It lives in `.settings/` which is
gitignored, so everyone sets it once.

## 5. Flashing Debug

```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/SCU2.0.hex -rst
```

Or use a CubeIDE debug config pointed at `build/Debug/SCU2.0.elf`.

## Adding source files

Add them to `target_sources()` in the root `CMakeLists.txt`. CubeMX only tracks
files it generates.

Do not edit `cmake/stm32cubemx/CMakeLists.txt`, CubeMX overwrites it.

## Debug vs Release

Debug is `-O0 -g3`, Release is `-Os`. Use Debug on the bench, Release on the
car. Test Release separately, optimization changes timing and stack usage.

## Troubleshooting

Build folder is disposable:

```bash
rm -rf build/Debug && cmake --preset Debug && cmake --build build/Debug
```

**`can_utils.h` not found:** run the submodule command in step 2.

**CubeIDE and CLI disagree:** check the PATH step.

**Sensor reads exactly zero:** its file is probably missing from
`CMakeLists.txt`.
