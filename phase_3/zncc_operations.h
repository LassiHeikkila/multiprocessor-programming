#ifndef _ZNCC_OPERATIONS_H_
#define _ZNCC_OPERATIONS_H_

#include <stdint.h>

#include "coord_fifo.h"
#include "types.h"

void extract_window(
    double        *in,
    double        *out,
    const uint32_t x_offset,
    const uint32_t y_offset,
    const uint32_t win_width,
    const uint32_t win_height,
    const uint32_t in_width,
    const uint32_t in_height
);

double calculate_window_mean(double *img, const uint32_t W, const uint32_t H);

double calculate_window_standard_deviation(
    double *img, const uint32_t W, const uint32_t H, const double window_mean
);

void zero_mean_window(
    double *img, const uint32_t W, const uint32_t H, const double window_mean
);

void normalize_window(
    double *img, const uint32_t W, const uint32_t H, const double std_dev
);

double window_dot_product(
    double *left, double *right, const uint32_t W, const uint32_t H
);

int32_t find_nearest_nonzero_neighbour(
    int32_t      *img,
    uint32_t      W,
    uint32_t      H,
    uint32_t      x,
    uint32_t      y,
    uint8_t      *visited,
    coord_fifo_t *fifo
);

#endif  // _ZNCC_OPERATIONS_H_