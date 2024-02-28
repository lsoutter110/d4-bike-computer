cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(arcana_lcd_rp2040 C CXX ASM)

pico_sdk_init()

add_library(arcana_lcd_rp2040 lcd_util.h lcd_misc.c lcd_draw.c lcd_text.c lcd_hl.h)

pico_generate_pio_header(arcana_lcd_rp2040 ${CMAKE_CURRENT_LIST_DIR}/lcd_driver.pio)

target_link_libraries(arcana_lcd_rp2040 PUBLIC
    pico_stdlib
    hardware_pio
)

target_include_directories(arcana_lcd_rp2040 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

# target_sources(arcana_lcd_rp2040 PUBLIC
#     lcd_hl.h
#     lcd_misc.c    
# )
