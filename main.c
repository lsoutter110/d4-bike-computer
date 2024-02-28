#include "lcd_hl.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/regs/intctrl.h"
#include "hardware/timer.h"

#include <stdio.h>

#define DATA_PINS 8
#define CTRL_PINS 17
#define CS 19
#define BLC 20
#define RESET 21
// #define VSYNC 26
// #define FMARK 27
#define FREQ 10000000

#define UART_FS uart0
#define UART_FS_IRQ UART0_IRQ
#define UART_FS_TIMEOUT_US 2000
#define UART_SS uart1
#define UART_SS_IRQ UART1_IRQ
#define UART_FS_BAUD 38400
#define UART_SS_BAUD 115200
#define UART_FS_TX 0
#define UART_FS_RX 1
#define UART_SS_TX 4
#define UART_SS_RX 5

#define BT1_PIN 27
#define BT2_PIN 28

static inline uint64_t get_micros() {
    const uint32_t l = timer_hw->timelr;
    const uint32_t h = timer_hw->timehr;
    return (((uint64_t)h << 32) | l) / 125;
}

void sensor_uart_init(uart_inst_t *uart, const uint baud, const uint tx_pin, const uint rx_pin, uint irq, irq_handler_t irq_handler) {
    uart_init(uart, baud);
    gpio_init(tx_pin);
    gpio_init(rx_pin);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    // Enable interrupts for rx buffers
    uart_set_irq_enables(uart, true, false);
    irq_set_exclusive_handler(irq, irq_handler);
    irq_set_enabled(irq, true);
}

typedef struct {
    float force;
    uint32_t time;
} ForceData;

void on_uart_fs_rx() {
    static uint8_t mode = 0; // 0 = waiting, 1 = receiving
    static uint8_t type; // Packet header type
    static uint8_t p_cnt; // Packet count (data segment)
    static uint8_t data[sizeof(ForceData)];
    static uint64_t last_read = 0;

    const uint8_t byte = (uint8_t) uart_getc(UART_FS);
    const uint64_t cur_time = get_micros();
    const uint64_t delta = cur_time - last_read;

    last_read = cur_time;

    printf("RX %02X\n", byte);

    if(!mode || delta > UART_FS_TIMEOUT_US) {
        if(delta > UART_FS_TIMEOUT_US) printf("UART timeout! last packet was %lluus ago\n", delta);
        printf("Start packet\n");
        mode = 1;
        type = byte;
        p_cnt = 0;
        return;
    }

    switch(type) {
    case 0x00:
        // Buffer data
        data[p_cnt++] = byte;
        if(p_cnt >= sizeof(ForceData)) {
            const ForceData fd = *(ForceData *)data;
            printf("Received force %.000fN and time %dms\n", fd.force, fd.time);
            gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
            mode = 0;
        }
        return;
    }
}

int main() {
    stdio_init_all();

    printf("stdio initialised\n");

    const LCD lcd = lcd_init(pio0, DATA_PINS, CTRL_PINS, CS, BLC, RESET, FREQ);
    
    lcd_draw_rgb_triangle(lcd, 0, 0, 240);
    
    // Initialise force sensor uart to receive sensor data
    sensor_uart_init(UART_FS, UART_FS_BAUD, UART_FS_TX, UART_FS_RX, UART_FS_IRQ, &on_uart_fs_rx);
    // sensor_uart_init(UART_SS, UART_SS_BAUD, UART_SS_TX, UART_SS_RX);

    // Initialise button pins
    gpio_init(BT1_PIN);
    gpio_init(BT2_PIN);
    gpio_set_dir(BT1_PIN, GPIO_IN);
    gpio_set_dir(BT1_PIN, GPIO_IN);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while(1) {
        printf("loop %d\n", uart_is_readable(UART_FS));
        sleep_ms(1000);
    }
}