# Sensor Control Unit (SCU)

## Introduction
This repository contains the code for the Sensor Control Unit (SCU) of the Formula Electric SAE Student Team UMSAE Electric.
The SCU is responsible for the acquisition of data from the sensors in the vehicle, such as the temperature sensors, the shock potentiometers, the flowmeter, and the Inertial Measurement Unit (IMU).
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

## Building the code
The code is built using STM32CubeIDE.

### Prerequisites
- STM32CubeIDE
- STM32CubeMX (optional)

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