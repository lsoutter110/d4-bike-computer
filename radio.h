#ifndef RADIO_H
#define RADIO_H

#include "hardware/regs/intctrl.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define ZETA_TX 4
#define ZETA_RX 5
#define ZETA_SDN 6
#define ZETA_CHANNEL 2
#define ZETA_BYTES 5
#define ZETA_UART uart1
#define ZETA_IRQ UART1_IRQ
#define ZETA_RX_BUFSIZE 8
#define ZETA_RX_TIMEOUT 10000

#define ZETA_PACKET_HEADER 4
typedef struct __attribute__((__packed__)) {
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

typedef enum {
    PACKET_TYPE_OFFSET          = 0x00,
    PACKET_TYPE_COEFF           = 0x01,
    PACKET_TYPE_WHEEL_R         = 0x02,
    PACKET_TYPE_P_TO_S0         = 0x10,
    PACKET_TYPE_P_TO_S1         = 0x11,
    PACKET_TYPE_P_TO_S2         = 0x12,
    PACKET_TYPE_P_TO_S3         = 0x13,
    PACKET_TYPE_POSITION        = 0x21,
    PACKET_TYPE_TARGET_SPEED    = 0x22,
    PACKET_TYPE_OPEN            = 0x30,
    PACKET_TYPE_CLOSE           = 0x31,
    PACKET_TYPE_POWER           = 0x80,
    PACKET_TYPE_SPEED           = 0x81,
    PACKET_TYPE_ADJ_SPEED       = 0x82,
    PACKET_TYPE_CADENCE         = 0x83,
    PACKET_TYPE_GET_PARAM       = 0x90,
    PACKET_TYPE_ACK_OPEN        = 0x91,
    PACKET_TYPE_ACK_CLOSE       = 0x92,
} packet_type_t;

void init_zeta_callback();
void zeta_callback();
void radio_rx_buf_push(radio_packet_t p);
radio_packet_t radio_rx_buf_pop();
bool radio_rx_buf_readable();
void decode_packet(radio_packet_t p);
void send_packet(radio_packet_t p);
void update_config();
void post_status();

// EXISTS IN MAIN.CPP
void send_packet(radio_packet_t p);

#endif