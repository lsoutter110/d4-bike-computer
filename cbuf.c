#include "cbuf.h"
#include <stdio.h>

buf_fs_t buf_fs;
buf_ss_t buf_ss;

void bufs_init() {
    buf_fs.head = buf_fs.buf;
    buf_fs.tail = buf_fs.buf;

    buf_ss.head = buf_ss.buf;
    buf_ss.tail = buf_ss.buf;
}

void buf_fs_push(const force_data_t fd) {
    // push and increment ptr
    *(buf_fs.head) = fd;
    buf_fs.head++;
    // check for buffer overflow and overrun
    if(buf_fs.head >= buf_fs.buf+FS_BUFSIZE)
        buf_fs.head = buf_fs.buf;
    if(buf_fs.head == buf_fs.tail)
        buf_fs.tail++;
    if(buf_fs.tail >= buf_fs.buf+FS_BUFSIZE)
        buf_fs.tail = buf_fs.buf;
}

void buf_ss_push(const uint32_t time) {
    // push and increment ptr
    *(buf_ss.head++) = time;
    // check for buffer overflow and overrun
    if(buf_ss.head >= buf_ss.buf+SS_BUFSIZE)
        buf_ss.head = buf_ss.buf;
    if(buf_ss.head == buf_ss.tail)
        buf_ss.tail++;
    if(buf_ss.tail >= buf_ss.buf+SS_BUFSIZE)
        buf_ss.tail = buf_ss.buf;
}