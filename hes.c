#include "hes.h"
#include "cbuf.h"
#include "uart.h"
#include "interface.h"
#include <stdio.h>

void hes_callback(uint gpio, uint32_t event_mask) {
    buf_ss_push(get_micros()/1000);
    recalc_speed();
}

void init_hes(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_input_enabled(pin, true);
    gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &hes_callback);
}


/*
GET WATER
TAKE SAW FOR STRIPBOARD
TAKE SCALE!!!!!!!
TAKE ETH SWITCH
*/