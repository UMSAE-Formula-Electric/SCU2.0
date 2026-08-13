# Sensor Control Unit (SCU)

## Introduction
This repository contains the code for the Sensor Control Unit (SCU) of the Formula Electric SAE Student Team UMSAE Electric.
The SCU is responsible for the acquisition of data from the sensors in the vehicle, such as
* Shock potentiometers
* Cooling loop thermistors
* Brake temperature thermocouples
* Wheel speed sensors
* Flowmeter 

The SCU is also responsible for the communication with the other ECUs in the vehicle, such as the Accumulator Control Unit (ACU) and the Vehicle Control Unit (VCU).

## Cloning the repository
### Option 1: Using `git clone --recurse-submodules` (for newer Git versions)
```bash
git clone --recurse-submodules git@github.com:UMSAE-Formula-Electric/SCU2.0.git
```

### Option 2: Using `git clone` and `git submodule update --init --recursive` (for older Git versions)
```bash
git clone git@github.com:UMSAE-Formula-Electric/SCU2.0.git
cd SCU2.0
git submodule update --init --recursive
```

### Option 3: If you have already cloned the repository without submodules
```bash
cd SCU2.0
git submodule update --init --recursive
```

## Building and Flashing (New CMake Setup)

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


## Documentation
[Documentation](https://docs.google.com/document/d/1yh-vWpTT_wejGpQy9ZQ7YVPmWP_CSU35yJfaNo2MAhA/edit?usp=drive_link)

## Sensor Datasheets
### Thermistor

[**Temperature Sensors Line Guide**](https://drive.google.com/file/d/1dFKre9OKWxstRY-8zY3tSeb5LHlalR9f/view?usp=drive_link)

[**Honeywell ES110 and ES120 Series**](https://drive.google.com/file/d/1ghP8LBqkBAeznuTnMJgH6ZwvHYQ8_Wdi/view?usp=drive_link)

[**STEINHART & HART Equation**](https://drive.google.com/file/d/1xewpkaQVt0oJnFVwyUym_0Ixe8qXAvnR/view?usp=drive_link)

### Shock Pot
Uses ADC1 to read the voltages and convert them to distances.

[**Shock Pot Datasheet**](https://drive.google.com/file/d/1g9wjH6BT5--y21_IYlu2G4MbX3KbiAo5/view?usp=share_link)

### Flowmeter
Uses TIM12 Channel 1 (pin PB14) to capture the pulses from the flowmeter and determine the flowrate of the fluid in m^3/s.

[**Flowmeter Datasheet**](https://drive.google.com/file/d/1eDgrtt3bu5jgN7wbFmfX5pk9RuAkB_hs/view?usp=share_link)

[**Alternate Flowmeter Datasheet**](https://drive.google.com/file/d/1jgxNPx9BC58do8RVcqYU6hQfMw6DpTo8/view?usp=share_link)

[**Flowmeter Setup Example**](https://drive.google.com/file/d/1NjP9FMI18lw6DNG9UNYPWaCROAbHNBPB/view?usp=share_link)