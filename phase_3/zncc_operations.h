#ifndef _ZNCC_OPERATIONS_H_
#define _ZNCC_OPERATIONS_H_

#include <stdint.h>

#include "types.h"

void extract_window(
    gray_t        *in,
    int32_t       *out,
    const uint32_t x_offset,
    const uint32_t y_offset,
    const uint32_t win_width,
    const uint32_t win_height,
    const uint32_t in_width,
    const uint32_t in_height
);

int32_t calculate_window_mean(int32_t *img, const uint32_t W, const uint32_t H);

int32_t calculate_window_standard_deviation(
    int32_t *img, const uint32_t W, const uint32_t H, const int32_t window_mean
);

void zero_mean_window(
    int32_t *img, const uint32_t W, const uint32_t H, const int32_t window_mean
);

void normalize_window(
    int32_t *img, const uint32_t W, const uint32_t H, const int32_t std_dev
);

int32_t sum_of_elementwise_multiply_windows(
    int32_t *left, int32_t *right, const uint32_t W, const uint32_t H
);

#endif  // _ZNCC_OPERATIONS_H_