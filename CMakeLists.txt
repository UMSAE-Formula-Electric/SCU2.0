cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME SCU2.0)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Sources CubeMX does not generate. See CMAKE_SETUP.md.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/brake_temp.c
    Core/Src/flowmeter.c
    Core/Src/imu.c
    Core/Src/shock_pot.c
    Core/Src/thermistor.c
    Core/Src/wheel_speed.c
    Core/ThreadSafe/newlib_lock_glue.c
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE Core/ThreadSafe)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/Firmware-Library/Src/can_utils.c")
    message(FATAL_ERROR
        "Firmware-Library submodule is empty. Run:\n"
        "  git submodule update --init --recursive")
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Firmware-Library/Src/bms_can_utils.c
    Firmware-Library/Src/can_utils.c
    Firmware-Library/Src/car_state.c
    Firmware-Library/Src/error_handler.c
    Firmware-Library/Src/logger.c
    Firmware-Library/Src/motor_controller_can_utils.c
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE Firmware-Library/Inc)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    --specs=nano.specs
    -fstack-usage
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    "SHELL:-u _printf_float"
    "SHELL:-u _scanf_float"
)

set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")

target_link_libraries(${CMAKE_PROJECT_NAME}
    -Wl,--start-group c m -Wl,--end-group
)

if(NOT CMAKE_SIZE)
    find_program(CMAKE_SIZE NAMES arm-none-eabi-size REQUIRED)
endif()

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_PROJECT_NAME}.bin
    COMMENT "Size report + generating .hex and .bin"
)
