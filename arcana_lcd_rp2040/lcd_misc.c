#include "lcd_hl.h"
#include "lcd_util.h"

LCD lcd_controller_init(const PIO pio, const uint data_pins, const uint ctrl_pins, const uint cs,
    const uint blc, const uint reset, const float freq)
{
    // claim state machine for controller and initialise PIO
    const uint sm = pio_claim_unused_sm(pio, true);
    const uint offset = pio_add_program(pio, &lcd_driver_program);
    lcd_driver_program_init(pio, sm, offset, data_pins, ctrl_pins, freq);

    // setup control pins
    // gpio_init_mask(lcd.data_mask);
    // gpio_init(lcd.wr);
    // gpio_init(lcd.rd);
    // gpio_init(lcd.rs);
    gpio_init(cs);
    gpio_init(blc);
    gpio_init(reset);
	// gpio_init(vsync);
	// gpio_init(fmark);

    // gpio_set_dir_masked(lcd.data_mask, GPIO_OUT);
    // gpio_set_dir(lcd.wr, GPIO_OUT);
    // gpio_set_dir(lcd.rd, GPIO_OUT);
    // gpio_set_dir(lcd.rs, GPIO_OUT);
    gpio_set_dir(cs, GPIO_OUT);
    gpio_set_dir(blc, GPIO_OUT);
    gpio_set_dir(reset, GPIO_OUT);
    // gpio_set_dir(vsync, GPIO_OUT);
    // gpio_set_dir(fmark, GPIO_OUT);

    // gpio_put(lcd.wr, 1);
    // gpio_put(lcd.rd, 1);
    // gpio_put(lcd.rs, 1);
    gpio_put(cs, 0);
    gpio_put(reset, 1);
	// gpio_put(vsync, 1);
	// gpio_put(fmark, 1);

    return (LCD) { pio, sm, data_pins, ctrl_pins, cs, blc, reset, };
}

LCD lcd_init(const PIO pio, const uint data_pins, const uint ctrl_pins, const uint cs,
    const uint blc, const uint reset, const float freq)
{
	const LCD lcd = lcd_controller_init(pio, data_pins, ctrl_pins, cs, blc, reset, freq);

    sleep_ms(20);
	//perform reset
	lcd_reset_sleep_out(lcd);

	lcd_cmd_w(lcd, LCD_DISPLAY_OFF);
	//exit sleep
	lcd_cmd_w(lcd, LCD_SLEEP_OUT);
	sleep_ms(5); //wait for sleep out to finish

	// ----- setup preferences -----

	//set orientation
	lcd_cmd_w(lcd, LCD_MEMORY_ACCESS_CONTROL);
	lcd_data_w(lcd, 0x48);

	//set color depth to 16 bit {REG 3A : DBI[2:0] = 0b101}
	lcd_cmd_w(lcd, LCD_PIXEL_FORMAT_SET);
	lcd_data_w(lcd, 0x55);

	//clear display
	lcd_clear(lcd);

	//turn on display after reset
	lcd_cmd_w(lcd, LCD_DISPLAY_ON);
	sleep_ms(150);
	
	//enable backlight
	lcd_backlight_on(lcd);

    return lcd;
}

void lcd_draw_rgb_triangle(const LCD lcd, const int x, const int y, const int w) {
    //test triangle
    const uint16_t rb_max = 0x1F, r_off = 11;
    const uint16_t g_max = 0x3F, g_off = 5;
    const uint16_t               b_off = 0;

    lcd_mem_write_begin(lcd, x, y, x+w-1, y+w-1);
    for(int yr=0; yr<w; yr++) {
		int16_t g = g_max - g_max*yr/w;
		int16_t w_yr_2 = (w-yr)/2;
        for(int xr=0; xr<w; xr++) {
            if(2*xr + yr < w || yr + w < 2*xr) {
                lcd_mem_write_16(lcd, 0);
                continue;
            }
            const int16_t r = rb_max - rb_max*(xr + w_yr_2)/w;
            const int16_t b = rb_max*(xr - w_yr_2)/w;
			const uint16_t col = r<<r_off | g<<g_off | b<<b_off;
            lcd_mem_write_16(lcd, col);
        }
	}
    lcd_mem_write_end(lcd);
}