#include "zncc_operations.h"

#include <math.h>

void extract_window(
    gray_t        *in,
    int32_t       *out,
    const uint32_t x_offset,
    const uint32_t y_offset,
    const uint32_t win_width,
    const uint32_t win_height,
    const uint32_t in_width,
    const uint32_t in_height
) {
    uint32_t y, x;
    uint32_t rx = 0;
    uint32_t ry = 0;
    uint32_t ox = 0;
    uint32_t oy = 0;

    // clang-format off
    for (
        y = y_offset - ((win_height - 1) / 2);
        y < (y_offset + ((win_height - 1) / 2));
        ++y
    ) {
        // clang-format on

        ry = y;
        if (ry < 0) {
            ry = 0;
        } else if (y > (in_height - 1)) {
            ry = in_height - 1;
        }

        // clang-format off
        for (
            x = x_offset - ((win_width - 1) / 2);
            x < (x_offset + ((win_width - 1) / 2));
            ++x
        ) {
            // clang-format on

            rx = x;
            if (rx < 0) {
                rx = 0;
            } else if (x > (in_width - 1)) {
                rx = in_width - 1;
            }

            out[(oy * win_width) + ox] = (int32_t)in[(ry * in_width) + rx];

            ++ox;
        }
        ++oy;
        ox = 0;
    }
}

int32_t calculate_window_mean(
    int32_t *img, const uint32_t W, const uint32_t H
) {
    int32_t total = 0;

    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            total += img[(y * W) + x];
        }
    }

    return total / (W * H);
}

int32_t calculate_window_standard_deviation(
    int32_t *img, const uint32_t W, const uint32_t H, const int32_t window_mean
) {
    int32_t deviation         = 0;
    int32_t squared_deviation = 0;
    int32_t variance_sum      = 0;
    double  variance          = 0;
    double  std_dev           = 0.0;

    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            deviation         = img[(y * W) + x] - window_mean;
            squared_deviation = deviation * deviation;

            variance_sum += squared_deviation;
        }
    }

    variance = (double)variance_sum / (double)(W * H);

    std_dev = sqrt(variance);

    return (int32_t)std_dev;
}

void zero_mean_window(
    int32_t *img, const uint32_t W, const uint32_t H, const int32_t window_mean
) {
    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            img[(y * W) + x] = img[(y * W) + x] - window_mean;
        }
    }
}

void normalize_window(
    int32_t *img, const uint32_t W, const uint32_t H, const int32_t std_dev
) {
    if (std_dev == 0) {
        return;
    }

    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            img[(y * W) + x] = img[(y * W) + x] / std_dev;
        }
    }
}

int32_t sum_of_elementwise_multiply_windows(
    int32_t *left, int32_t *right, const uint32_t W, const uint32_t H
) {
    int32_t sum = 0;
    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            sum += left[(y * W) + x] * right[(y * W) + x];
        }
    }
    return sum;
}