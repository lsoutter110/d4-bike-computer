/* 
 * RP2040 port of the Arcana LCD library (ili934X driver)
 * 
 * Uses the 8-bit parallel interface of the ili934X, and a PIO to send/recieve data
 * lcd_util is mostly a collection of macros and small functions to make working with the ili934X easier.
 * Include lcd_util.h (which requires lcd_pio.h) for a low level interface. For a higher level interface,
 * build the library with make and include arcana_lcd.h and .a
 * 
 * 
 * 
*/
#pragma once

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "lcd_driver.pio.h"

#define LCD_WIDTH 240
#define LCD_HEIGHT 320
#define LCD_BG_COL 0x0000

// Basic Commands
enum {
	LCD_NO_OPERATION				= 0x00,
	LCD_SOFTWARE_RESET				= 0x01,
	LCD_READ_DISPLAY_ID				= 0x04,
	LCD_READ_DISPLAY_STATUS			= 0x09,
	LCD_READ_DISPLAY_POWER_MODE		= 0x0A,
	LCD_READ_DISPLAY_MADCTL			= 0x0B,
	LCD_READ_DISPLAY_PIXEL_FORMAT	= 0x0C,
	LCD_READ_DISPLAY_IMAGE_FORMAT	= 0x0D,
	LCD_READ_DISPLAY_SIGNAL_MODE	= 0x0E,
	LCD_RAED_DISPLAY_SELF_DIAG_RSLT	= 0x0F,
	LCD_ENTER_SLEEP_MODE			= 0x10,
	LCD_SLEEP_OUT					= 0x11,
	LCD_PARTIAL_MODE_ON				= 0x12,
	LCD_NORMAL_DISPLAY_MODE_ON		= 0x13,
	LCD_DISPLAY_INVERSION_OFF		= 0x20,
	LCD_DISPLAY_INVERSION_ON		= 0x21,
	LCD_GAMMA_SET					= 0x26,
	LCD_DISPLAY_OFF					= 0x28,
	LCD_DISPLAY_ON					= 0x29,
	LCD_COLUMN_ADDRESS_SET			= 0x2A,
	LCD_PAGE_ADDRESS_SET			= 0x2B,
	LCD_MEMORY_WRITE				= 0x2C,
	LCD_COLOR_SET					= 0x2D,
	LCD_MEMORY_READ					= 0x2E,
	LCD_PARTIAL_AREA				= 0x30,
	LCD_VERTICAL_SCROLLING_DEF		= 0x33,
	LCD_TEARING_EFFECT_LINE_OFF		= 0x34,
	LCD_TEARING_EFFECT_LINE_ON		= 0x35,
	LCD_MEMORY_ACCESS_CONTROL		= 0x36,
	LCD_VERTICAL_SCROLL_START_ADR	= 0x37,
	LCD_IDLE_MODE_OFF				= 0x38,
	LCD_IDLE_MODE_ON				= 0x39,
	LCD_PIXEL_FORMAT_SET			= 0x3A,
	LCD_WRITE_MEMORY_CONTINUE		= 0x3C,
	LCD_READ_MEMORY_CONTINUE		= 0x3E,
	LCD_SET_TEAR_SCANLINE			= 0x44,
	LCD_GET_SCANLINE				= 0x45,
	LCD_WRITE_DISPLAY_BRIGHTNESS	= 0x51,
	LCD_READ_DISPLAY_BRIGHTNESS		= 0x52,
	LCD_WRITE_CTRL_DISPLAY			= 0x53,
	LCD_READ_CTRL_DISPLAY			= 0x54,
	LCD_WRITE_CABC					= 0x55,
	LCD_READ_CABC					= 0x56,
	LCD_WRITE_CABC_MIN_BRIGHTNESS	= 0x5E,
	LCD_READ_CABC_MIN_BRIGHTNESS	= 0x5F,
	LCD_READ_ID1					= 0xDA,
	LCD_READ_ID2					= 0xDB,
	LCD_READ_ID3					= 0xDC,
};

// Control macros
#define lcd_cs_en(lcd) gpio_put(lcd.cs, 0)
#define lcd_cs_dis(lcd) gpio_put(lcd.cs, 1)
#define lcd_backlight_on(lcd) gpio_put(lcd.blc, 1)
#define lcd_backlight_off(lcd) gpio_put(lcd.blc, 0)
#define lcd_reset(lcd) do { sleep_us(10); gpio_put(lcd.reset, 0); sleep_us(20); gpio_put(lcd.reset, 1); sleep_ms(5); } while(0)
#define lcd_reset_sleep_out(lcd) do { sleep_us(10); gpio_put(lcd.reset, 0); sleep_us(20); gpio_put(lcd.reset, 1); sleep_ms(120); } while(0)

// R/W macros
#define lcd_cmd_w(lcd, cmd) pio_sm_put_blocking((lcd).pio, (lcd).sm, (cmd)|0x000)
#define lcd_data_w(lcd, data) pio_sm_put_blocking((lcd).pio, (lcd).sm, ((data)&0xFF)|0x100)
#define lcd_data_w_repeat(lcd, data, repeats) pio_sm_put_blocking((lcd).pio, (lcd).sm, ((data)&0xFF)|0x100|((repeats)<<9))
/*
#define lcd_cmd_w(lcd, cmd) \
do { \
    gpio_put(lcd.rs, 0); sleep_us(1); \
    gpio_put(lcd.wr, 0); sleep_us(1); \
    gpio_put_masked(lcd.data_mask, (cmd)<<lcd.data_pins); sleep_us(1); \
    gpio_put(lcd.wr, 1); sleep_us(1); \
    gpio_put(lcd.rs, 1); sleep_us(1); \
} while(0)

#define lcd_data_w(lcd, data) \
do { \
    gpio_put(lcd.wr, 0); sleep_us(1); \
    gpio_put_masked(lcd.data_mask, (data)<<lcd.data_pins); sleep_us(1); \
    gpio_put(lcd.wr, 1); sleep_us(1); \
} while(0)
*/

// drawing macros
#define lcd_set_col_adr(lcd, x1, x2) \
do {\
	lcd_cmd_w(lcd, LCD_COLUMN_ADDRESS_SET);\
	lcd_data_w(lcd, (x1)>>8);\
	lcd_data_w(lcd, x1);\
	lcd_data_w(lcd, (x2)>>8);\
	lcd_data_w(lcd, x2);\
} while(0)

#define lcd_set_page_adr(lcd, y1, y2) \
do {\
	lcd_cmd_w(lcd, LCD_PAGE_ADDRESS_SET);\
	lcd_data_w(lcd, (y1)>>8);\
	lcd_data_w(lcd, y1);\
	lcd_data_w(lcd, (y2)>>8);\
	lcd_data_w(lcd, y2);\
} while(0)

#define lcd_mem_write_begin(lcd, x1, y1, x2, y2) \
do {\
	lcd_set_col_adr(lcd, x1, x2);\
	lcd_set_page_adr(lcd, y1, y2);\
	lcd_cmd_w(lcd, LCD_MEMORY_WRITE);\
} while(0)

#define lcd_mem_write_8(lcd, d) lcd_data_w(lcd, d)
#define lcd_mem_write_16(lcd, d) do { lcd_data_w(lcd, (d)>>8); lcd_data_w(lcd, d); } while(0)
#define lcd_mem_write_end(lcd) lcd_cmd_w(lcd, LCD_NO_OPERATION)
