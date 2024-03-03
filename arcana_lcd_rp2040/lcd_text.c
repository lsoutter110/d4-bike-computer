#include "lcd_hl.h"
#include "lcd_util.h"

void lcd_write_char(const LCD lcd, const Font font, char c, const int x, const int y, const uint16_t col, const uint16_t bg) {
    if(c < 32) c = 127;
    const uint8_t *data_ptr = font.data + ((size_t)(c-32))*font.char_offset;
    uint8_t mask = 0x80;
    uint8_t buf = *(data_ptr++);
    lcd_mem_write_begin(lcd, x, y, x+font.w-1, y+font.h-1);
    for(int yr=y; yr<y+font.h; yr++)
        for(int xr=x; xr<x+font.w; xr++) {
            if(mask & buf) {
                lcd_mem_write_16(lcd, col);
            } else {
                lcd_mem_write_16(lcd, bg);
            }
            
            mask >>= 1;
            if(mask == 0) {
                mask = 0x80;
                buf = *(data_ptr++);
            }
        }
    lcd_mem_write_end(lcd);
}

uint lcd_write_str(const LCD lcd, const Font font, const char *str, const int x1, const int y1, const int x2, const int y2, const uint16_t col, const uint16_t bg) {
    const int xi = font.w + font.x_space;
    const int yi = font.h + font.y_space;
    int x = x1;
    int y = y1;
    for(char c; c = *str; str++) {
        if(y > y2 - yi)
            return 1;
        if(c == '\n') {
            x = x1;
            y += yi;
            continue;
        }
        if(x > x2 - xi) {
            x = x1;
            y += yi;
        }
        lcd_write_char(lcd, font, c, x, y, col, bg);
        x += xi;
    }
    return 0;
}

void lcd_write_char_scale(const LCD lcd, const Font font, char c, const int x, const int y, const uint16_t col, const uint16_t bg, const int x_scale, const int y_scale) {
    if(c < 32) c = 127;
    const uint8_t *data_ptr = font.data + ((size_t)(c-32))*font.char_offset;
    uint8_t mask = 0x80;
    uint8_t buf = *(data_ptr++);
    lcd_mem_write_begin(lcd, x, y, x+font.w*x_scale-1, y+font.h*y_scale-1);
    for(int yr=y; yr<y+font.h; yr++) {
        const uint8_t *const row_ptr = data_ptr;
        const uint8_t row_mask = mask;
        const uint8_t row_buf = buf;
        for(int ys=0; ys<y_scale; ys++) {
            data_ptr = row_ptr;
            mask = row_mask;
            buf = row_buf;
            for(int xr=x; xr<x+font.w; xr++) {
                if(mask & buf) {
                    for(int xs=0; xs<x_scale; xs++)
                        lcd_mem_write_16(lcd, col);
                } else {
                    for(int xs=0; xs<x_scale; xs++)
                        lcd_mem_write_16(lcd, bg);
                }
                
                mask >>= 1;
                if(mask == 0) {
                    mask = 0x80;
                    buf = *(data_ptr++);
                }
            }
        }
    }
    lcd_mem_write_end(lcd);
}

uint lcd_write_str_scale(const LCD lcd, const Font font, const char *str, const int x1, const int y1, const int x2, const int y2, const uint16_t col, const uint16_t bg, const int x_scale, const int y_scale) {
    const int xi = (font.w + font.x_space)*x_scale;
    const int yi = (font.h + font.y_space)*y_scale;
    const int w = font.w*x_scale;
    const int h = font.w*y_scale;
    int x = x1;
    int y = y1;
    for(char c; c = *str; str++) {
        if(y + h - 1 > y2)
            return 1;
        if(c == '\n') {
            x = x1;
            y += yi;
            continue;
        }
        if(x + w - 1 > x2) {
            x = x1;
            y += yi;
        }
        lcd_write_char_scale(lcd, font, c, x, y, col, bg, x_scale, y_scale);
        x += xi;
    }
    return 0;
}