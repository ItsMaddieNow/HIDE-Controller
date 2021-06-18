# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/pi/pico/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(HIDE-Controller C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(HIDE-Controller src/HIDE-Controller.cpp)

target_include_directories(HIDE-Controller PRIVATE include/)

pico_set_program_name(HIDE-Controller "HIDE-Controller")
pico_set_program_version(HIDE-Controller "0.1")

pico_enable_stdio_uart(HIDE-Controller 1)
pico_enable_stdio_usb(HIDE-Controller 0)

# Add the standard library to the build
target_link_libraries(HIDE-Controller pico_stdlib)

# Add any user requested libraries
target_link_libraries(HIDE-Controller
        hardware_adc
        hardware_interp
        hardware_timer
        hardware_clocks
        tinyusb_device
        tinyusb_board
        )

pico_add_extra_outputs(HIDE-Controller)

