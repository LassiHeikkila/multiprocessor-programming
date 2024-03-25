#include "coord_fifo.h"

#include <stddef.h>

coord_t *coord_fifo_dequeue(coord_fifo_t *fifo) {
    if (fifo == NULL) {
        return NULL;
    }
    if (fifo->read == fifo->write) {
        return NULL;
    }
    coord_t *ret = &(fifo->storage[fifo->read]);
    fifo->read   = ((fifo->read + 1) % fifo->capacity);
    return ret;
}

int32_t coord_fifo_enqueue(coord_fifo_t *fifo, coord_t coord) {
    if (fifo == NULL) {
        return -1;
    }
    if (((fifo->write + 1) % fifo->capacity) == fifo->read) {
        return -1;
    }
    fifo->storage[fifo->write] = coord;
    fifo->write                = ((fifo->write + 1) % fifo->capacity);
    return 0;
}

uint32_t coord_fifo_len(coord_fifo_t *fifo) {
    if (fifo == NULL) {
        return 0;
    }
    return ((fifo->write - fifo->read) % fifo->capacity);
}
