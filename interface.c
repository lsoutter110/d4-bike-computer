#include "interface.h"

#include "cbuf.h"
#include <math.h>
#include <stdio.h>
#include "arcana_lcd_rp2040/default5x7.h"
#include <string.h>
#include <stdio.h>
#include "radio.h"
#include <math.h>

typedef struct {
    char *text;
    screen_t link;
} menu_item_t;

extern buf_fs_t buf_fs;
extern buf_ss_t buf_ss;

// Calculations
float power;
float speed;
float cadence;
float target_power;
float adjusted_speed;

config_t config;
screen_t cur_screen;
uint32_t redraw_flags;
uint menu_pos;
radio_packet_t last_packet;
force_data_t last_force_packet;

const menu_item_t MAIN_MENU_ITEMS[] = {
    {"Race", SCREEN_RACE},
    {"Debug", SCREEN_DEBUG}
};
const uint MENU_LEN = sizeof(MAIN_MENU_ITEMS)/sizeof(menu_item_t);

debug_stats_t debug_stats;

void recalc_target_power() {
    if(config.p_to_s[config.position])
        target_power = config.target_speed / config.p_to_s[config.position];
}

void recalc_adjusted_speed() {
    adjusted_speed = power * config.p_to_s[config.position];
}

#define mod_buf_ss(x) ((x)>=SS_BUFSIZE ? (x)-SS_BUFSIZE : (x))

void recalc_speed() {
    // diff buffer and average
    uint32_t diffs[SS_BUFSIZE-1];
    uint diff_n = 0;
    uint64_t mean = 0;
    const uint start = buf_ss.tail-buf_ss.buf;
    const uint end = (buf_ss.head == buf_ss.buf ? SS_BUFSIZE : buf_ss.head-buf_ss.buf) - 1;
    for(uint i=start; i!=end; i=mod_buf_ss(i+1), diff_n++) {
        const uint32_t diff = buf_ss.buf[mod_buf_ss(i+1)]-buf_ss.buf[i];
        diffs[diff_n] = diff;
        mean += diff;
    }
    // Check there are actually readings to process
    if(!diff_n) return;
    mean /= diff_n;

    // reject out of threshold diffs
    const uint32_t upper = mean * 1.75;
    const uint32_t lower = mean * 0.5;
    uint32_t count = 0;
    bool accepted;
    mean = 0;
    for(int i=0; i<diff_n; i++) {
        accepted = diffs[i] > lower && diffs[i] < upper;
        if(accepted) {
            mean += diffs[i];
            count++;
        }
    }
    mean /= count;
    // speed = (2*pi*wheel_r) / (time_ms / 1000)
    speed = 2.0*M_PI*1000.0*config.wheel_r / mean;

    // Update debug stats
    if(accepted)
        debug_stats.speed_accepted_readings++;
    else
        debug_stats.speed_rejected_readings++;

    // Update display draw
    redraw_flags |= REDRAW_FLAG_DEBUG | REDRAW_FLAG_SPEED;
}

#define mod_buf_fs(x) ((x)>=FS_BUFSIZE ? (x)-FS_BUFSIZE : (x))

void recalc_cadence() {
    // Make local buffer copy
    float f[FS_BUFSIZE];
    float t[FS_BUFSIZE];
    const uint start = buf_fs.tail-buf_fs.buf;
    const uint end = buf_fs.head-buf_fs.buf;
    uint bufsize = 0;
    for(uint i=start, j=0; i!=end; i=mod_buf_fs(i+1), j++) {
        f[j] = buf_fs.buf[i].force;
        t[j] = buf_fs.buf[i].time;
        bufsize++;
    }

    // Get RMS
    float f_rms = 0.0;
    for(uint i=0; i<bufsize; i++) {
        f_rms += f[i]*f[i];
    }
    f_rms = sqrtf(f_rms/bufsize);
    const float f_upper_th = f_rms;
    const float f_lower_th = f_rms/2;

    // Run smoothing algorithm
    float f_sum = 0.0;
    const float kernel[] = {0.1, 0.1, 0.1, 0.1, 0.2, 0.1, 0.1, 0.1, 0.1};
    const uint k_len = sizeof(kernel)/sizeof(float);
    float f_smoothed[FS_BUFSIZE];
    for(int i=0; i<bufsize; i++) {
        float fs = 0;
        for(int ki=0; ki<k_len; ki++) {
            const int fi = i+ki-(k_len-1)/2;
            if(fi >= 0 && fi < bufsize) {
                fs += f[fi]*kernel[ki];
            }
        }
        f_smoothed[i] = fs;
    }

    // Rise / fall detection
    bool peak = false;
    uint32_t last_rise = 0;
    uint32_t diff_sum = 0;
    uint diff_n = 0;
    for(uint fi=0; fi<bufsize; fi++) {
        if(!peak && f_smoothed[fi] > f_upper_th) {
            peak = true;
            if(last_rise != 0) {
                diff_sum += t[fi] - last_rise;
                diff_n++;
            }
            last_rise = t[fi];
        }

        if(peak && f_smoothed[fi] < f_lower_th) {
            peak = false;
        }
    }

    // Calculate cadence (cadence=60/avg_period)
    cadence = 60.0*1000.0*(float)diff_n/(float)diff_sum;
}

static inline void draw_split_rect(const LCD lcd, const int x1, const int y1, const int x2, const int y2, const int hs, const uint16_t lower_col, const uint16_t upper_col) {
    const int ys = hs > y2-y1+1 ? y1-1 : (hs < 0 ? y2 : y2-hs);
    lcd_draw_rect_fill(lcd, x1, y1, x2, ys, upper_col);
    lcd_draw_rect_fill(lcd, x1, ys+1, x2, y2, lower_col);
}

void redraw_speed(const LCD lcd) {
    if(cur_screen != SCREEN_RACE)
        return;    

    static const int PBAR_X = 140, BAR_W = 80, PBAR_Y = 20, BAR_H = 240;
    static const int MAX_SPEED = 20;
    static const int BORDER_W = 4;
    const uint16_t BORDER_COL = rgb_to_u16(255, 255, 255);
    const uint16_t BAR_BG_COL = rgb_to_u16(0,0,0);
    const uint16_t TARGET_COL = rgb_to_u16(0,128,255);
    const uint16_t LOWER_COL = rgb_to_u16(0, 255, 0);
    const uint16_t UPPER_COL = rgb_to_u16(255, 0, 0);

    const int speed_target_h = config.target_speed*(BAR_H-2*BORDER_W)/MAX_SPEED;
    const int speed_target_y = PBAR_Y+BAR_H-BORDER_W-1-speed_target_h;
    const int speed_h = speed*(BAR_H-2*BORDER_W)/MAX_SPEED;

    // Draw speed bar
    lcd_draw_rect_fill(lcd, PBAR_X, PBAR_Y, PBAR_X+BORDER_W-1, PBAR_Y+BAR_H-1, BORDER_COL);
    lcd_draw_rect_fill(lcd, PBAR_X+BAR_W-BORDER_W, PBAR_Y, PBAR_X+BAR_W-1, PBAR_Y+BAR_H-1, BORDER_COL);
    lcd_draw_rect_fill(lcd, PBAR_X, PBAR_Y, PBAR_X+BAR_W-1, PBAR_Y+BORDER_W-1, BORDER_COL);
    lcd_draw_rect_fill(lcd, PBAR_X, PBAR_Y+BAR_H-BORDER_W, PBAR_X+BAR_W-1, PBAR_Y+BAR_H-1, BORDER_COL);

    draw_split_rect(lcd, PBAR_X+BORDER_W, speed_target_y+1, PBAR_X+BAR_W-BORDER_W-1, PBAR_Y+BAR_H-BORDER_W-1, speed_h, LOWER_COL, BAR_BG_COL);
    
    draw_split_rect(lcd, PBAR_X+BORDER_W, PBAR_Y+BORDER_W, PBAR_X+BAR_W-BORDER_W-1, speed_target_y-BORDER_W, speed_h-speed_target_h-BORDER_W+1, UPPER_COL, BAR_BG_COL);

    lcd_draw_rect_fill(lcd, PBAR_X+BORDER_W, speed_target_y-BORDER_W+1, PBAR_X+BAR_W-BORDER_W-1, speed_target_y, TARGET_COL);
}

void redraw_power(const LCD lcd) {
    if(cur_screen != SCREEN_RACE)
        return;

    static const int PBAR_X = 20, BAR_W = 80, PBAR_Y = 20, BAR_H = 240;
    static const int MAX_POWER = 500;
    static const int BORDER_W = 4;
    const uint16_t BORDER_COL = rgb_to_u16(255, 255, 255);
    const uint16_t BAR_BG_COL = rgb_to_u16(0,0,0);
    const uint16_t TARGET_COL = rgb_to_u16(0,128,255);
    const uint16_t LOWER_COL = rgb_to_u16(0, 255, 0);
    const uint16_t UPPER_COL = rgb_to_u16(255, 0, 0);

    const int power_target_h = target_power*(BAR_H-2*BORDER_W)/MAX_POWER;
    const int power_target_y = PBAR_Y+BAR_H-BORDER_W-1-power_target_h;
    const int power_h = power*(BAR_H-2*BORDER_W)/MAX_POWER;

    // Draw power bar
    lcd_draw_rect_fill(lcd, PBAR_X, PBAR_Y, PBAR_X+BORDER_W-1, PBAR_Y+BAR_H-1, BORDER_COL);
    lcd_draw_rect_fill(lcd, PBAR_X+BAR_W-BORDER_W, PBAR_Y, PBAR_X+BAR_W-1, PBAR_Y+BAR_H-1, BORDER_COL);
    lcd_draw_rect_fill(lcd, PBAR_X, PBAR_Y, PBAR_X+BAR_W-1, PBAR_Y+BORDER_W-1, BORDER_COL);
    lcd_draw_rect_fill(lcd, PBAR_X, PBAR_Y+BAR_H-BORDER_W, PBAR_X+BAR_W-1, PBAR_Y+BAR_H-1, BORDER_COL);

    draw_split_rect(lcd, PBAR_X+BORDER_W, power_target_y+1, PBAR_X+BAR_W-BORDER_W-1, PBAR_Y+BAR_H-BORDER_W-1, power_h, LOWER_COL, BAR_BG_COL);
    
    draw_split_rect(lcd, PBAR_X+BORDER_W, PBAR_Y+BORDER_W, PBAR_X+BAR_W-BORDER_W-1, power_target_y-BORDER_W, power_h-power_target_h-BORDER_W+1, UPPER_COL, BAR_BG_COL);

    lcd_draw_rect_fill(lcd, PBAR_X+BORDER_W, power_target_y-BORDER_W+1, PBAR_X+BAR_W-BORDER_W-1, power_target_y, TARGET_COL);
}

// Change screen
void change_screen(const screen_t new_screen) {
    // reset menu to top
    menu_pos = 0;
    cur_screen = new_screen;

    redraw_flags = -1; // set all flags
}

void redraw_all(const LCD lcd) {
    lcd_clear(lcd);

    redraw_speed(lcd);
    redraw_power(lcd);
    
    redraw_main_menu(lcd);

    redraw_debug(lcd);
}

void redraw_main_menu(const LCD lcd) {
    if(cur_screen != SCREEN_MAIN_MENU)
        return;
    
    static const int ITEM_X = 40;
    static const int ITEM_INIT_Y = 20;
    static const int ITEM_W = 160;
    static const int ITEM_H = 20;
    static const int TEXT_SCALE = 2;
    static const int Y_PADDING = 4;
    static const uint16_t BORDER_COL = 0xFFFF;
    static const uint16_t TEXT_COL = 0xFFFF;
    static const uint16_t ITEM_BG_COL = 0x4208;
    static const uint16_t SEL_BORDER_COL = 0xC7FF;
    static const uint16_t SEL_TEXT_COL = 0x0000;
    static const uint16_t SEL_ITEM_BG_COL = 0x43EF;

    int ypos = ITEM_INIT_Y;
    for(uint i=0; i<MENU_LEN; i++) {
        const uint16_t border_col = i == menu_pos ? SEL_BORDER_COL : BORDER_COL;
        const uint16_t text_col = i == menu_pos ? SEL_TEXT_COL : TEXT_COL;
        const uint16_t item_bg_col = i == menu_pos ? SEL_ITEM_BG_COL : ITEM_BG_COL;

        // Draw menu item
        lcd_draw_hline(lcd, ITEM_X, ypos, ITEM_X+ITEM_W-1, border_col);
        lcd_draw_hline(lcd, ITEM_X, ypos+ITEM_H-1, ITEM_X+ITEM_W-1, border_col);
        lcd_draw_vline(lcd, ITEM_X, ypos, ypos+ITEM_H-1, border_col);
        lcd_draw_vline(lcd, ITEM_X+ITEM_W-1, ypos, ypos+ITEM_H-1, border_col);

        lcd_draw_rect_fill(lcd, ITEM_X+1, ypos+1, ITEM_X+ITEM_W-2, ypos+ITEM_H-2, item_bg_col);

        const int text_w = (strlen(MAIN_MENU_ITEMS[i].text)*(default5x7.w+default5x7.x_space) - default5x7.x_space)*TEXT_SCALE;
        const int text_h = default5x7.h*TEXT_SCALE;
        const int text_x = ITEM_X + ITEM_W/2 - text_w/2;
        const int text_y = ypos + ITEM_H/2 - text_h/2;
        lcd_write_str_scale(lcd, default5x7, MAIN_MENU_ITEMS[i].text, text_x, text_y,
            text_x+text_w-1, text_y+text_h-1, text_col, item_bg_col, TEXT_SCALE, TEXT_SCALE);

        ypos += ITEM_H + Y_PADDING;
    }
}

// Debug screen macros
#define BUFSIZE 4096
#define push_buf(c) buf[bufptr++] = c
#define push_str(s) for(char *_i=s; *_i; _i++) push_buf(*i);

void redraw_debug(const LCD lcd) {
    if(cur_screen != SCREEN_DEBUG)
        return;    
    
    char buf[4096];
    size_t bufptr = 0;

    float total_events = debug_stats.speed_accepted_readings + debug_stats.speed_rejected_readings;
    float det_rate = (debug_stats.speed_accepted_readings*100)/total_events;
    bufptr += sprintf(buf+bufptr,
        "Speed: %3.2fm/s, stats:\n- Accepted: %d\n- Rejected: %d\n- Detection rate: %3.1f%%\n",
        speed, debug_stats.speed_accepted_readings, debug_stats.speed_rejected_readings, det_rate
    );

    bufptr += sprintf(buf+bufptr, "Adjusted speed: %3.2f\n", adjusted_speed);
    bufptr += sprintf(buf+bufptr, "Cadence: %3.2f\n", cadence);
    bufptr += sprintf(buf+bufptr, "Power: %3.2fW\n", power);

    bufptr += sprintf(buf+bufptr, "RX: {%02X %08X}\n", last_packet.type, last_packet.data.i);

    bufptr += sprintf(buf+bufptr, "Force: {%fN %u}\n", last_force_packet.force, last_force_packet.time);
    float f_mean = 0.0;
    for(uint i=0; i<FS_BUFSIZE; i++) f_mean += buf_fs.buf[i].force;
    bufptr += sprintf(buf+bufptr, "- buffer mean: %fN\n", f_mean/FS_BUFSIZE);

    bufptr += sprintf(buf+bufptr,
        "config = {\n  offset: %u\n  coeff: %f\n  wheel r: %f\n  position: %u\n  target speed: %f\n  connection open: %u\n}\n",
        config.fs_offset,
        config.fs_coeff,
        config.wheel_r,
        config.position,
        config.target_speed,
        config.connection_open
    );

    // Write buffer to screen
    push_buf(0);
    lcd_clear(lcd);
    lcd_write_str(lcd, default5x7, buf, 0, 0, 240, 320, 0xFFFF, 0x0000);
}

void reset_debug_stats() {
    debug_stats = (debug_stats_t) {
        0,      // speed_accepted_readings
        0,      // speed_rejected_readings
    };

    //calibrate load cell
    float f_mean = 0.0;
    for(uint i=0; i<FS_BUFSIZE; i++) f_mean += buf_fs.buf[i].force;
    config.fs_offset += f_mean/FS_BUFSIZE/config.fs_coeff;
}

void init_interface_buttons() {
    gpio_set_input_enabled(BT_MENU, true);
    gpio_init(BT_MENU);
    gpio_set_dir(BT_MENU, GPIO_IN);
    gpio_set_input_hysteresis_enabled(BT_MENU, true);
    gpio_set_irq_enabled(BT_MENU, GPIO_IRQ_EDGE_RISE, true);
    
    gpio_set_input_enabled(BT_CYCLE, true);
    gpio_init(BT_CYCLE);
    gpio_set_dir(BT_CYCLE, GPIO_IN);
    gpio_set_input_hysteresis_enabled(BT_CYCLE, true);
    gpio_set_irq_enabled(BT_CYCLE, GPIO_IRQ_EDGE_RISE, true);

    gpio_set_irq_callback(&bt_callback);
    irq_set_enabled(IO_IRQ_BANK0, true);
    
}

void bt_callback(uint gpio, uint32_t event) {
    gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));

    if(gpio == BT_MENU) {
        // Menu/select button functionality
        if(cur_screen != SCREEN_MAIN_MENU) {
            change_screen(SCREEN_MAIN_MENU);
        } else {
            change_screen(MAIN_MENU_ITEMS[menu_pos].link);
        }
    }

    if(gpio == BT_CYCLE) {
        // Cycle button functionality
        if(++menu_pos == MENU_LEN)
            menu_pos = 0;
        if(cur_screen == SCREEN_DEBUG)
            reset_debug_stats();
        redraw_flags |= REDRAW_FLAG_MAIN_MENU | REDRAW_FLAG_DEBUG;
    }
}