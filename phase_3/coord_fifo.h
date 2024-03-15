#ifndef _COORD_FIFO_H_
#define _COORD_FIFO_H_

#include <stdint.h>

typedef struct coord {
    uint32_t x;
    uint32_t y;
} coord_t;

typedef struct coord_fifo {
    coord_t *storage;
    uint32_t read;
    uint32_t write;
    uint32_t capacity;
} coord_fifo_t;

coord_t *coord_fifo_dequeue(coord_fifo_t *fifo);

int32_t coord_fifo_enqueue(coord_fifo_t *fifo, coord_t coord);

uint32_t coord_fifo_len(coord_fifo_t *fifo);

#endif  // _COORD_FIFO_H_