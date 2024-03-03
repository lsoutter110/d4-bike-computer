cmake_minimum_required(VERSION 3.13)

# Pre-build scripts
include(build_scripts/pre_build.cmake)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(bike_comp_firmware C CXX ASM)
set(CMAKE_CXX_STANDARD 20)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_subdirectory(arcana_lcd_rp2040)
add_subdirectory(hun_zeta)

add_executable(bike_comp_firmware
    main.cpp
    uart.c
    interface.c
    cbuf.c
    hes.c
    radio.c
)

pico_generate_pio_header(bike_comp_firmware ${CMAKE_CURRENT_LIST_DIR}/uart_tx.pio)
pico_generate_pio_header(bike_comp_firmware ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(bike_comp_firmware PUBLIC pico_stdlib hardware_uart hardware_timer pico_multicore arcana_lcd_rp2040 hun_zeta)

# enable usb output, disable uart output
pico_enable_stdio_usb(bike_comp_firmware 1)
pico_enable_stdio_uart(bike_comp_firmware 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(bike_comp_firmware)
