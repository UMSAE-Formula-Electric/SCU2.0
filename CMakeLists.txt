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

# =============================================================================
# USER ADDITIONS - carried over from the STM32CubeIDE build
# =============================================================================
# Everything below covers what CubeMX does NOT know about. Deliberately minimal:
# the toolchain file already supplies -mcpu/-mfpu/-mfloat-abi, -Wall -Wextra
# -Wpedantic, -O0 -g3 (Debug) / -Os (Release), --specs=nano.specs, --gc-sections
# and the libc/libm link group, and cmake/stm32cubemx supplies USE_HAL_DRIVER,
# STM32F446xx, STM32_THREAD_SAFE_STRATEGY=4 and the 58 generated sources.
#
# The Eclipse build compiled 65 files. cmake/stm32cubemx lists 52. These are
# the missing 13:
#
#   Core/Src sensor drivers   6   hand-written, not CubeMX-generated peripherals
#   Core/ThreadSafe           1   CubeIDE feature, was configured in .cproject
#   Firmware-Library/Src      6   git submodule, outside CubeMX's world
#                            --
#                            13
# =============================================================================

# -----------------------------------------------------------------------------
# Sensor drivers - 6 sources
# -----------------------------------------------------------------------------
# CubeMX generates peripheral setup (adc.c, tim.c, can.c ...) but knows nothing
# about hand-written application code that sits alongside it in Core/Src.
#
# Omitting these fails loudly at link time - "undefined reference to
# StartReadTempTask" and friends, because freertos.c registers them as task
# entry points. That is the good case. Contrast with Core/ThreadSafe below,
# where omission produces NO error at all.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/brake_temp.c
    Core/Src/flowmeter.c
    Core/Src/imu.c
    Core/Src/shock_pot.c
    Core/Src/thermistor.c
    Core/Src/wheel_speed.c
)

# -----------------------------------------------------------------------------
# Firmware-Library (git submodule) - 6 sources CubeMX has no knowledge of
# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# Thread-safe newlib glue - 1 source
# -----------------------------------------------------------------------------
# cmake/stm32cubemx defines STM32_THREAD_SAFE_STRATEGY=4 but does NOT compile
# the file that implements it. Without this, newlib's own no-op lock stubs are
# linked instead and concurrent printf/malloc across FreeRTOS tasks corrupts
# shared state - with no build error of any kind.
#
# CubeMX also drops copies of newlib_lock_glue.c / stm32_lock.h in the repo
# root but never references them. Core/ThreadSafe/ is used here instead: it is
# the newer (2024) template and matches the verification baseline. The two
# differ only in licence header comments.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/ThreadSafe/newlib_lock_glue.c
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE Core/ThreadSafe)

# -----------------------------------------------------------------------------
# Float printf - MUST accompany --specs=nano.specs
# -----------------------------------------------------------------------------
# The toolchain file sets --specs=nano.specs, which compiles float support out
# of printf/scanf to save space. The logging path depends on it everywhere
# (thermistor.c, brake_temp.c, flowmeter.c, shock_pot.c, imu.c all use %f).
# Without these, every %f prints EMPTY - no compile error, no link error.
#
# The "SHELL:" prefix is REQUIRED here. Without it CMake de-duplicates the
# repeated "-u" token and emits "-u _printf_float _scanf_float", so the linker
# treats _scanf_float as an input filename:
#     ld.exe: cannot find _scanf_float: No such file or directory
# SHELL: keeps each flag+value pair together as one unsplittable argument.
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    "SHELL:-u _printf_float"
    "SHELL:-u _scanf_float"
)

# -----------------------------------------------------------------------------
# Per-function stack usage reports (.su files), as the Eclipse build produced
# -----------------------------------------------------------------------------
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -fstack-usage)

# -----------------------------------------------------------------------------
# --specs=nano.specs AT COMPILE TIME - not just at link time
# -----------------------------------------------------------------------------
# ST's toolchain file passes nano.specs only in CMAKE_C_LINK_FLAGS. The Eclipse
# build passed it on every compile command as well, and that difference is not
# cosmetic:
#
#   nano.specs defines _REENT_SMALL, which selects a much smaller layout for
#   newlib's "struct _reent".
#
# Link-only means every translation unit compiles against the FULL struct
# layout while being linked against newlib-nano, which was built with the small
# one. Any code touching a struct _reent field then reads the wrong offsets.
# Core/ThreadSafe/newlib_lock_glue.c takes "struct _reent *" arguments
# directly, so this lands squarely on the thread-safety path.
#
# Visible symptom: FreeRTOS TCBs embed a struct _reent when
# configUSE_NEWLIB_REENTRANT=1, so Idle_TCB/Timer_TCB grew 168 -> 384 bytes
# each, inflating .bss by exactly 432 bytes versus the Eclipse baseline.
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE --specs=nano.specs)

# -----------------------------------------------------------------------------
# Pin the Release optimisation level explicitly
# -----------------------------------------------------------------------------
# ST's toolchain file puts "-Os -g0" in CMAKE_C_FLAGS for Release, but CMake
# appends CMAKE_C_FLAGS_<CONFIG> AFTER CMAKE_C_FLAGS, and its built-in default
# for Release is "-O3 -DNDEBUG". Last -O flag wins, so ST's -Os was being
# silently overridden and Release actually built at -O3.
#
# -O3 is numerically safe (it does NOT imply -ffast-math, unlike -Ofast), but
# its aggressive inlining grows stack frames - and FreeRTOS tasks here have
# fixed stack sizes, so that is a real overflow risk on the build that goes in
# the car.
#
# -Os matches what the toolchain author intended. Change to -O2 if you want
# more speed and have measured the stack headroom.
# -DNDEBUG is kept: it is CMake's Release convention and disables assert().
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")

# -----------------------------------------------------------------------------
# Post-build: size report and flashable images
# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# C library group - MUST come after the objects
# -----------------------------------------------------------------------------
# target_link_libraries() populates <LINK_LIBRARIES>, which CMake emits after
# the object files. target_link_options() would NOT work here: those land in
# <LINK_FLAGS>, ahead of the objects, which is exactly the bug this fixes.
#
# With -lc scanned before our objects, newlib's own libc_a-lock.o satisfied the
# __retarget_lock_* symbols first and collided with newlib_lock_glue.c. Do not
# "fix" that with --allow-multiple-definition: the first definition wins, so
# newlib's no-op stubs would silently replace real FreeRTOS locking.
target_link_libraries(${CMAKE_PROJECT_NAME}
    -Wl,--start-group c m -Wl,--end-group
)

# =============================================================================
# PER-CONFIGURATION SETTINGS  (Debug vs Release)
# =============================================================================
# Exactly two configurations exist, defined in CMakePresets.json:
#
#   cmake --preset Debug    -> build/Debug     CMAKE_BUILD_TYPE=Debug
#   cmake --preset Release  -> build/Release   CMAKE_BUILD_TYPE=Release
#
# Preset name, directory name and CMAKE_BUILD_TYPE are all identical on
# purpose. Avoid inventing custom build-type names: ST's
# cmake/gcc-arm-none-eabi.cmake keys off the standard ones
# ("if(CMAKE_BUILD_TYPE MATCHES Release)" is what supplies -Os -g0), so a type
# called e.g. "Prod" would match nothing and silently build unoptimised.
#
# What each config gives you today (from the toolchain file):
#     Debug     -O0 -g3   + DEBUG defined (from cmake/stm32cubemx)
#     Release   -Os -g0
#
# ---------------------------------------------------------------------------
# HOW TO MAKE THE BUILDS DIFFER
# ---------------------------------------------------------------------------
# The pattern is a generator expression wrapped around any target_* setting.
# It is evaluated per-configuration at build time, so one CMakeLists serves
# both builds.
#
#   Define a macro in one config only:
#       target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#           $<$<CONFIG:Release>:PROD_BUILD>
#           $<$<CONFIG:Debug>:VERBOSE_SENSOR_LOGS>
#       )
#
#   Give a macro different VALUES per config:
#       target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#           $<$<CONFIG:Debug>:LIBRARY_LOG_LEVEL=LOG_DEBUG>
#           $<$<CONFIG:Release>:LIBRARY_LOG_LEVEL=LOG_NONE>
#       )
#
#   Different compile options:
#       target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
#           $<$<CONFIG:Release>:-ffunction-sections -flto>
#       )
#
#   Compile a source into ONE config only (no generator expression needed -
#   plain if() on CMAKE_BUILD_TYPE works because this is a single-config
#   generator):
#       if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#           target_sources(${CMAKE_PROJECT_NAME} PRIVATE Core/Src/debug_shell.c)
#       endif()
#
# ---------------------------------------------------------------------------
# CURRENT STATE: intentionally identical
# ---------------------------------------------------------------------------
# Debug and Prod differ ONLY in optimisation/debug-info, which comes from the
# toolchain file. No behavioural divergence yet - that keeps the port
# verifiable against the Eclipse baseline.
#
# When you are ready to strip serial logging from the car build, the change is
# the block below plus three edits in the logger (see cmake-port/README.md):
#
# target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#     $<$<CONFIG:Debug>:LIBRARY_LOG_LEVEL=LOG_DEBUG>
#     $<$<CONFIG:Release>:LIBRARY_LOG_LEVEL=LOG_NONE>
# )
#
# Do that as its own commit, after the port is bench-verified.
