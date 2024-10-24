#include "uart.h"

#include "hardware/timer.h"
#include <stdio.h>

#include "uart_tx.pio.h"
#include "uart_rx.pio.h"
#include "cbuf.h"
#include "interface.h"

pio_uart_t fs_uart;
pio_uart_t ss_uart;

extern force_data_t last_force_packet;
extern config_t config;

uint64_t get_micros() {
    const uint32_t l = timer_hw->timelr;
    const uint32_t h = timer_hw->timehr;
    return ((uint64_t)h << 32) | l;
}

void sensor_uart_init(PIO pio, const uint baud, const uint fs_tx_pin, const uint fs_rx_pin) {
    // shared tx/rx programs
    uint tx_offset = pio_add_program(pio, &uart_tx_program);
    uint rx_offset = pio_add_program(pio, &uart_rx_program);

    // Force sensor UART init
    uint fs_tx_sm = pio_claim_unused_sm(pio, true);
    uart_tx_program_init(pio, fs_tx_sm, tx_offset, fs_tx_pin, baud, 8);
    uint fs_rx_sm = pio_claim_unused_sm(pio, true);
    uart_rx_program_init(pio, fs_rx_sm, rx_offset, fs_rx_pin, baud, 8);

    // Enable interrupts for force sensor
    uint fs_irq = pio_get_index(pio) ? PIO1_IRQ_0 : PIO0_IRQ_0;
    pio_set_irqn_source_enabled(pio, 0, pis_sm0_rx_fifo_not_empty+fs_rx_sm, true);
    irq_set_exclusive_handler(fs_irq, on_uart_fs_rx);
    irq_set_enabled(fs_irq, true);

    // Populate global fs/ss UART structs
    fs_uart = (pio_uart_t) { pio, fs_tx_sm, pio, fs_rx_sm };
}

void on_uart_fs_rx() {
    static uint8_t mode = 0; // 0 = waiting, 1 = receiving
    static uint8_t type; // Packet header type
    static uint8_t p_cnt; // Packet count (data segment)
    static uint8_t data[sizeof(force_data_t)];
    static uint64_t last_read = 0;
    
    while(!pio_sm_is_rx_fifo_empty(fs_uart.rx_pio, fs_uart.rx_sm)) {
        const uint8_t byte = (uint8_t) pio_sm_get(fs_uart.rx_pio, fs_uart.rx_sm);
        const uint64_t cur_time = get_micros();
        const uint64_t delta = cur_time - last_read;

        last_read = cur_time;

        // printf("RX %02X, t = %lldms\n", byte, cur_time/1000);

        if(!mode || delta > UART_FS_TIMEOUT_US) {
            if(mode) printf("UART timeout! last packet was %lluus ago\n", delta);
            mode = 1;
            type = byte;
            p_cnt = 0;
            continue;
        }

        switch(type) {
        case 0x00:
            // Buffer data
            data[p_cnt++] = byte;
            if(p_cnt >= sizeof(force_data_t)) {
                force_data_t fd = *(force_data_t *)data;
                fd.force = (fd.force - config.fs_offset)*config.fs_coeff;
                last_force_packet = fd;
                buf_fs_push(fd);
                printf("Received force %.000fN and time %dms\n", fd.force, fd.time);
                gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
                mode = 0;
            }
            continue;
        }
    }
}
