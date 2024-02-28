#pragma once

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

// LCD definition
typedef struct {
    PIO pio;
    uint sm;
    uint data_pins;
	uint ctrl_pins;
    uint cs;
    uint blc;
    uint reset;
} LCD;

typedef struct {
    uint16_t *data;
    uint8_t w;
    uint16_t h;
} Sprite;

typedef struct {
    uint8_t *data;
    uint8_t w;
    uint16_t h;
} Mask;

typedef struct {
    const uint8_t *data;
    size_t char_offset;
    int w;
    int h;
    int x_space;
    int y_space;
} Font;

// initialisation
LCD lcd_init(const PIO pio, const uint data_pins, const uint ctrl_pins,
    const uint cs, const uint blc, const uint reset, const float freq);

// drawing functions
void lcd_clear(const LCD lcd);
void lcd_draw_hline(const LCD lcd, const int x1, const int y1, const int x2,
    const uint16_t col);
void lcd_draw_vline(const LCD lcd, const int x1, const int y1, const int y2,
    const uint16_t col);
void lcd_draw_rect_fill(const LCD lcd, const int x1, const int y1, const int x2,
    const int y2, const uint16_t col);
void lcd_draw_line(const LCD lcd, const int x1, const int y1, const int x2,
    const int y2, const uint16_t col);

// Sprite functions
void lcd_draw_sprite(const LCD lcd, const Sprite sprite, const int x,
    const int y);
void lcd_draw_sprite_scale(const LCD lcd, const Sprite sprite, const int x,
    const int y, const int x_scale, const int y_scale);

// Mask functions
void lcd_draw_mask(const LCD lcd, const Mask mask, const int x, const int y,
    const uint16_t col1, const uint16_t col0);
void lcd_draw_mask_scale(const LCD lcd, const Mask mask, const int x,
    const int y, const uint16_t col1, const uint16_t col0, const int x_scale,
    const int y_scale);

// Text functions
void lcd_write_char(const LCD lcd, const Font font, char c, const int x, 
    const int y, const uint16_t col, const uint16_t bg);
uint lcd_write_str(const LCD lcd, const Font font, const char *str, 
    const int x1, const int y1, const int x2, const int y2, const uint16_t col, 
    const uint16_t bg);
void lcd_write_char_scale(const LCD lcd, const Font font, char c, const int x,
    const int y, const uint16_t col, const uint16_t bg, const int x_scale,
    const int y_scale);
uint lcd_write_str_scale(const LCD lcd, const Font font, const char *str,
    const int x1, const int y1, const int x2, const int y2, const uint16_t col,
    const uint16_t bg, const int x_scale, const int y_scale);

// Misc
void lcd_draw_rgb_triangle(const LCD lcd, const int x, const int y, const int w);
static inline uint16_t rgb_to_u16(const uint8_t r, const uint8_t g, const uint8_t b) {
    return (r<<8)&0xF800 | (g<<3)&0x07E0 | (b>>3);
}