#include "radio.h"
#include "uart.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/regs/intctrl.h"
#include "interface.h"
#include <stdio.h>

radio_rx_buf_t radio_rx_buf;

extern uint32_t redraw_flags;

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

        rx_data[rx_count++] = byte;

        if(rx_count == sizeof(radio_packet_t)) {
            radio_rx_buf_push(*(radio_packet_t *)rx_data);
            redraw_flags |= REDRAW_FLAG_DEBUG;
            rx_count = 0;
        }
    }
}