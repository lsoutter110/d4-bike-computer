#ifndef RADIO_H
#define RADIO_H

#include "hardware/regs/intctrl.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define ZETA_UART uart1
#define ZETA_IRQ UART1_IRQ
#define ZETA_RX_BUFSIZE 8
#define ZETA_RX_TIMEOUT 10000

typedef struct __attribute__((__packed__)) {
    char header_hash;
    char header_R;
    uint8_t header_bytes;
    uint8_t header_strength;
    uint8_t type;
    union {
        float f;
        uint32_t i;
    } data;
} radio_packet_t;

typedef struct {
    radio_packet_t buf[ZETA_RX_BUFSIZE];
    radio_packet_t *head;
    radio_packet_t *tail;
} radio_rx_buf_t;

void init_zeta_callback();
void zeta_callback();
void radio_rx_buf_push(radio_packet_t p);
radio_packet_t radio_rx_buf_pop();
bool radio_rx_buf_readable();

#endif