#include "lcd_hl.h"
#include "lcd_util.h"

#define abs(x) ((x)<0 ? -(x) : (x))
#define absn(x) ((x)<0 ? (x) : -(x))

// Drawing functions

void lcd_clear(const LCD lcd) {
	lcd_mem_write_begin(lcd, 0, 0, LCD_WIDTH-1, LCD_HEIGHT-1);

	for(int x=0; x<LCD_WIDTH; x++)
		for(int y=0; y<LCD_HEIGHT; y++)
			lcd_mem_write_16(lcd, LCD_BG_COL);
	
	lcd_mem_write_end(lcd);
}

void lcd_draw_hline(const LCD lcd, const int x1, const int y1, const int x2, const uint16_t col) {
	lcd_mem_write_begin(lcd, x1, y1, x2, y1);

	for(int x=x1; x<=x2; x++)
		lcd_mem_write_16(lcd, col);

	lcd_mem_write_end(lcd);
}

void lcd_draw_vline(const LCD lcd, const int x1, const int y1, const int y2, const uint16_t col) {
	lcd_mem_write_begin(lcd, x1, y1, x1, y2);

	for(int y=y1; y<=y2; y++)
		lcd_mem_write_16(lcd, col);

	lcd_mem_write_end(lcd);
}

void lcd_draw_rect_fill(const LCD lcd, const int x1, const int y1, const int x2, const int y2, const uint16_t col) {
	lcd_mem_write_begin(lcd, x1, y1, x2, y2);

	for(int x=x1; x<=x2; x++)
		for(int y=y1; y<=y2; y++)
			lcd_mem_write_16(lcd, col);

	lcd_mem_write_end(lcd);
}

void lcd_draw_line(const LCD lcd, const int x1, const int y1, const int x2, const int y2, const uint16_t col) {
    const int dx = abs(x1-x2);
    const int dy = absn(y1-y2);
    const int xi = x1<x2 ? 1 : -1;
    const int yi = y1<y2 ? 1 : -1;
    int err = dx + dy;
    int x = x1, y = y1;

    lcd_mem_write_begin(lcd, x, y, x, y);
    while(1) {
        lcd_mem_write_16(lcd, col);

        if(x == x2 && y == y2) break;
        
        if(2*err >= dy) {
            if(x == x2) break;
            err += dy;
            x += xi;
            lcd_set_col_adr(lcd, x, x);
            lcd_cmd_w(lcd, LCD_MEMORY_WRITE);
        }

        if(2*err <= dx) {
            if(y == y2) break;
            err += dx;
            y += yi;
            lcd_set_page_adr(lcd, y, y);
            lcd_cmd_w(lcd, LCD_MEMORY_WRITE);
        }
    }
    lcd_mem_write_end(lcd);
}