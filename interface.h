#ifndef INTERFACE_H
#define INTERFACE_H

#include "arcana_lcd_rp2040/lcd_hl.h"

#define BT_MENU 21
#define BT_CYCLE 22

typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_DEBUG,
    SCREEN_RACE,
} screen_t;

typedef struct {
    uint speed_accepted_readings;
    uint speed_rejected_readings;
} debug_stats_t;

// UI Updates
void change_screen(const screen_t new_screen);
void redraw_all(const LCD lcd);
void redraw_power(const LCD lcd);
void redraw_speed(const LCD lcd);
void redraw_main_menu(const LCD lcd);
void redraw_debug(const LCD lcd);

void recalc_power();
void recalc_speed();
void recalc_target_power();
void recalc_adjusted_speed();
void reset_debug_stats();

void init_interface_buttons();
void bt_callback(uint gpio, uint32_t event);

enum {
    REDRAW_FLAG_POWER       = 0x00000001,
    REDRAW_FLAG_SPEED       = 0x00000002,
    REDRAW_FLAG_MAIN_MENU   = 0x00000004,
    REDRAW_FLAG_DEBUG       = 0x00000008,
    REDRAW_FLAG_BG          = 0x00000010,
};

typedef struct {
    uint32_t fs_offset;
    float fs_coeff;
    float wheel_r;
    float p_to_s[4];
    uint32_t position;
    float target_speed;
    bool connection_open;
} config_t;

// 


#endif