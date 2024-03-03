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

#define ZETA_TX 4
#define ZETA_RX 5
#define ZETA_SDN 6
#define ZETA_CHANNEL 2
#define ZETA_BYTES 5

extern pio_uart_t fs_uart;
extern pio_uart_t ss_uart;
extern config_t config;
extern buf_fs_t buf_fs;
extern uint32_t redraw_flags;
extern float power;
extern float target_power;

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
    const zeta::config_t zeta_cfg = {
        zeta::uart_baud_opt::UART_19200,
        ZETA_RX, ZETA_TX, ZETA_SDN,
        ZETA_BYTES, ZETA_CHANNEL
    };
    zeta::transceiver ts(uart1, zeta_cfg);
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

    while(1) {
        // printf("loop %d\n", uart_is_readable(UART_FS));
        // sleep_ms(1000);
        // pio_sm_put_blocking(fs_uart.tx_pio, fs_uart.tx_sm, 0x5A);
        // pio_sm_put_blocking(ss_uart.tx_pio, ss_uart.tx_sm, 0xF3);
        // float mean = 0;
        // for(int i=0; i<FS_BUFSIZE; i++) {
        //     mean += buf_fs.buf[i].force;
        // }
        // mean /= FS_BUFSIZE;
        // mean = (mean - 497340.0)/4649.7016;
        // printf("%fN\n", mean);
        // sleep_ms(100);

        power = power > 500 ? 0 : power+10;
        target_power = target_power > 500 ? 0 : target_power+1;
        redraw_flags |= REDRAW_FLAG_POWER;
        sleep_ms(10);
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