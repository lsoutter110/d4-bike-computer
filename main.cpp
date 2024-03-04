extern "C" {
    #include "arcana_lcd_rp2040/lcd_hl.h"
    #include "uart.h"
    #include "interface.h"
    #include "cbuf.h"
    #include "hes.h"
    #include "radio.h"
}
// #include "zetaplus.h"
#include "hun_zeta/transceiver.h"

#include "pico/stdlib.h"
#include "hardware/regs/intctrl.h"
#include "pico/multicore.h"

#include <stdio.h>

#define DATA_PINS 8
#define CTRL_PINS 17
#define CS 18
#define BLC 19
#define RESET 20
// #define VSYNC 26
// #define FMARK 27
#define FREQ 10000000

#define HES_PIN 28

extern pio_uart_t fs_uart;
extern pio_uart_t ss_uart;
extern config_t config;
extern buf_fs_t buf_fs;
extern uint32_t redraw_flags;
extern float power;
extern float target_power;
extern radio_packet_t last_packet;

// Setup zetaplus radio
const zeta::config_t ZETA_CFG = {
    zeta::uart_baud_opt::UART_19200,
    ZETA_RX, ZETA_TX, ZETA_SDN,
    ZETA_BYTES, ZETA_CHANNEL
};
zeta::transceiver ts(ZETA_UART, ZETA_CFG);

void core1_main();

int main() {
    stdio_init_all();

    printf("stdio initialised\n");

    multicore_launch_core1(&core1_main);

    // Initialise force sensor uart to receive sensor data
    sensor_uart_init(pio1, UART_BAUD, UART_FS_TX, UART_FS_RX);
    bufs_init();

    init_hes(HES_PIN);
    
    // Setup ZETAPLUS Radio
    ts.set_rf_baud_rate(zeta::rf_baud_opt::RF_38400);
    ts.configure_rx(ZETA_BYTES, ZETA_CHANNEL);
    ts.request_firmware();
    zeta::response_t r = ts.read();
    printf("%s", r.firmware_str);
    init_zeta_callback();

    // Initialise debugging stats
    reset_debug_stats();

    // TEMPORARY: INITIALISE CONFIG
    config = (config_t) {
        0,      // offset
        1.0,    // coefficient
        1.0,    // wheel radius
        {1.0, 1.0, 1.0, 1.0}, // power to speed
        0,      // position
        10.0,   // target speed
    };

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    uint64_t last_status_post;
    const uint64_t next_status_post = 500000; //us

    while(1) {
        power = power > 500 ? 0 : power+10;
        target_power = target_power > 500 ? 0 : target_power+1;
        redraw_flags |= REDRAW_FLAG_POWER;
        sleep_ms(10);

        // Check rx buffer and process
        if(radio_rx_buf_readable()) {
            last_packet = radio_rx_buf_pop();
            decode_packet(last_packet);
        }

        if(config.connection_open && get_micros()-last_status_post > next_status_post) {
            post_status();
            last_status_post = get_micros();
        }
    }
}



void core1_main() {
    // Initialise button pins
    init_interface_buttons();

    const LCD lcd = lcd_init(pio0, DATA_PINS, CTRL_PINS, CS, BLC, RESET, FREQ);
    
    // Initialise the UI
    change_screen(SCREEN_MAIN_MENU);
    
    // Main draw loop
    while(1) {
        while(!redraw_flags) sleep_ms(20);

        uint32_t redraw_flags_capture = redraw_flags;
        redraw_flags = 0; // reset all flags

        if(redraw_flags_capture & REDRAW_FLAG_BG)
            lcd_clear(lcd);
        if(redraw_flags_capture & REDRAW_FLAG_POWER)
            redraw_power(lcd);
        if(redraw_flags_capture & REDRAW_FLAG_SPEED)
            redraw_speed(lcd);
        if(redraw_flags_capture & REDRAW_FLAG_MAIN_MENU)
            redraw_main_menu(lcd);
        if(redraw_flags_capture & REDRAW_FLAG_DEBUG)
            redraw_debug(lcd);
    }
}

// requires transceiver lib
// void send_packet(radio_packet_t p) {
//     ts.send_from<radio_packet_t>(p);
// }