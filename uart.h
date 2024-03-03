#ifndef UART_H
#define UART_H

#define UART_BAUD 38400
#define UART_FS_TX 0
#define UART_FS_RX 1
#define UART_FS_TIMEOUT_US 2000


#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include <stdint.h>

typedef struct {
    float force;
    uint32_t time;
} force_data_t;

typedef struct {
    PIO tx_pio;
    uint tx_sm;
    PIO rx_pio;
    uint rx_sm;
} pio_uart_t;

uint64_t get_micros();
void sensor_uart_init(PIO pio, const uint baud, const uint fs_tx_pin, const uint fs_rx_pin);
void on_uart_fs_rx();

#endif