#ifndef CBUF_H
#define CBUF_H

#include <stdlib.h>
#include "uart.h"

#define FS_BUFSIZE 50
#define SS_BUFSIZE 16

typedef struct {
    force_data_t buf[FS_BUFSIZE];
    force_data_t *head;
    force_data_t *tail;
} buf_fs_t;

typedef struct {
    uint32_t buf[SS_BUFSIZE];
    uint32_t *head;
    uint32_t *tail;
} buf_ss_t;

void bufs_init();
void buf_fs_push(const force_data_t fd);
void buf_ss_push(const uint32_t time);


#endif