cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK (must be set before project())
# Set PICO_SDK_PATH in your environment or pass -DPICO_SDK_PATH=... to cmake
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(reflow_oven C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(reflow_oven
    src/main.cpp
    src/lcd_api.cpp
    src/gpio_lcd.cpp
    src/LCD.cpp
    src/mcp9600.cpp
    src/pid.cpp
    src/ssr.cpp
    src/rotary.cpp
)

target_include_directories(reflow_oven PRIVATE src)

target_link_libraries(reflow_oven
    pico_stdlib
    hardware_i2c
    hardware_pwm
    hardware_gpio
    hardware_timer
    hardware_irq
)

# Enable USB serial output (UART also enabled)
pico_enable_stdio_usb(reflow_oven 1)
pico_enable_stdio_uart(reflow_oven 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(reflow_oven)
