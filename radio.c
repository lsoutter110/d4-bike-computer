#include "radio.h"
#include "uart.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/regs/intctrl.h"
#include "interface.h"
#include <stdio.h>

extern uint32_t redraw_flags;
extern config_t config;
extern float speed;
extern float power;
extern float adjusted_speed;
extern float cadence;

radio_rx_buf_t radio_rx_buf;

void radio_rx_buf_push(radio_packet_t p) {
    // push and increment ptr
    *(radio_rx_buf.head++) = p;
    // check for buffer overflow and overrun
    if(radio_rx_buf.head >= radio_rx_buf.buf+ZETA_RX_BUFSIZE)
        radio_rx_buf.head = radio_rx_buf.buf;
    if(radio_rx_buf.head == radio_rx_buf.tail)
        radio_rx_buf.tail++;
    if(radio_rx_buf.tail >= radio_rx_buf.buf+ZETA_RX_BUFSIZE)
        radio_rx_buf.tail = radio_rx_buf.buf;
}

radio_packet_t radio_rx_buf_pop() {
    // pop and increment tail
    const radio_packet_t ret = *(radio_rx_buf.tail++);
    if(radio_rx_buf.tail >= radio_rx_buf.buf+ZETA_RX_BUFSIZE)
        radio_rx_buf.tail = radio_rx_buf.buf;
    return ret;
}

bool radio_rx_buf_readable() {
    return radio_rx_buf.tail != radio_rx_buf.head;
}

void init_zeta_callback() {
    uart_set_irq_enables(ZETA_UART, true, false);
    irq_set_exclusive_handler(ZETA_IRQ, &zeta_callback);
    irq_set_enabled(ZETA_IRQ, true);

    // Reset buffer
    radio_rx_buf.head = radio_rx_buf.buf;
    radio_rx_buf.tail = radio_rx_buf.buf;
}

void zeta_callback() {
    // Packet rx handler for radio
    static uint8_t rx_data[sizeof(radio_packet_t)];
    static uint8_t rx_count;
    static uint64_t last_read = 0;

    printf("Zeta callback\n");

    while(uart_is_readable(ZETA_UART)) {
        const uint8_t byte = uart_getc(ZETA_UART);
        const uint64_t cur_time = get_micros();
        const uint64_t delta = cur_time - last_read;
        last_read = cur_time;
        printf("Read byte %02X at %llums\n", byte, cur_time);

        if(rx_count != 0 && delta > ZETA_RX_TIMEOUT) {
            printf("Zeta RX Timeout!\n");
            rx_count = 0;
        }
        
        if(rx_count >= ZETA_PACKET_HEADER)
            rx_data[rx_count-ZETA_PACKET_HEADER] = byte;
        rx_count++;

        if(rx_count >= sizeof(radio_packet_t)+ZETA_PACKET_HEADER) {
            radio_rx_buf_push(*(radio_packet_t *)rx_data);
            rx_count = 0;
        }
    }
}

void decode_packet(const radio_packet_t p) {
    redraw_flags |= REDRAW_FLAG_DEBUG;
    switch(p.type) {
        // Sensor calibration
        case PACKET_TYPE_OFFSET:
            config.fs_offset = p.data.i;
            return;
        case PACKET_TYPE_COEFF:
            config.fs_coeff = p.data.f;
            return;
        case PACKET_TYPE_WHEEL_R:
            config.wheel_r = p.data.f;
            return;
        // Power to speed coefficients
        case PACKET_TYPE_P_TO_S0:
        case PACKET_TYPE_P_TO_S1:
        case PACKET_TYPE_P_TO_S2:
        case PACKET_TYPE_P_TO_S3:
            config.p_to_s[p.type-PACKET_TYPE_P_TO_S0] = p.data.f;
            return;
        // Status / targets
        case PACKET_TYPE_POSITION:
            config.position = p.data.i;
            return;
        case PACKET_TYPE_TARGET_SPEED:
            config.target_speed = p.data.f;
            return;
        // Misc
        case PACKET_TYPE_OPEN:
            config.connection_open = true;
            send_packet((radio_packet_t) {PACKET_TYPE_ACK_OPEN, .data.i=0});
            update_config();
            return;
        case PACKET_TYPE_CLOSE:
            config.connection_open = false;
            send_packet((radio_packet_t) {PACKET_TYPE_ACK_CLOSE, .data.i=0});
            return;
    }
}

void send_packet(const radio_packet_t p) {
    uart_putc(ZETA_UART, 'A');
    uart_putc(ZETA_UART, 'T');
    uart_putc(ZETA_UART, 'S');
    uart_putc(ZETA_UART, ZETA_CHANNEL);
    uart_putc(ZETA_UART, sizeof(radio_packet_t));
    for(uint i=0; i<sizeof(radio_packet_t); i++) {
        uart_putc(ZETA_UART, ((uint8_t *)&p)[i]);
    }
    // Wait for packet to send
    sleep_ms(10);
}

void update_config() {
    send_packet((radio_packet_t) {PACKET_TYPE_GET_PARAM, .data.i = PACKET_TYPE_OFFSET});
    send_packet((radio_packet_t) {PACKET_TYPE_GET_PARAM, .data.i = PACKET_TYPE_COEFF});
    send_packet((radio_packet_t) {PACKET_TYPE_GET_PARAM, .data.i = PACKET_TYPE_WHEEL_R});
    send_packet((radio_packet_t) {PACKET_TYPE_GET_PARAM, .data.i = PACKET_TYPE_POSITION});
    send_packet((radio_packet_t) {PACKET_TYPE_GET_PARAM, .data.i = PACKET_TYPE_TARGET_SPEED});
    send_packet((radio_packet_t) {PACKET_TYPE_GET_PARAM, .data.i = PACKET_TYPE_P_TO_S0});
    send_packet((radio_packet_t) {PACKET_TYPE_GET_PARAM, .data.i = PACKET_TYPE_P_TO_S1});
    send_packet((radio_packet_t) {PACKET_TYPE_GET_PARAM, .data.i = PACKET_TYPE_P_TO_S2});
    send_packet((radio_packet_t) {PACKET_TYPE_GET_PARAM, .data.i = PACKET_TYPE_P_TO_S3});
}

void post_status() {
    send_packet((radio_packet_t) {PACKET_TYPE_SPEED, .data.f = speed});
    send_packet((radio_packet_t) {PACKET_TYPE_POWER, .data.f = power});
    send_packet((radio_packet_t) {PACKET_TYPE_ADJ_SPEED, .data.f = adjusted_speed});
    send_packet((radio_packet_t) {PACKET_TYPE_CADENCE, .data.f = cadence});
}