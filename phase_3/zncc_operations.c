#include "zncc_operations.h"
#include "coord_fifo.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void extract_window(
    double        *in,
    double        *out,
    const uint32_t x_offset,
    const uint32_t y_offset,
    const uint32_t win_width,
    const uint32_t win_height,
    const uint32_t in_width,
    const uint32_t in_height
) {
    int32_t  y, x;
    int32_t  rx = 0;
    int32_t  ry = 0;
    uint32_t ox = 0;
    uint32_t oy = 0;

    // clang-format off
    for (
        y = (int32_t)y_offset - (((int32_t)win_height - 1) / 2);
        y <= ((int32_t)y_offset + (((int32_t)win_height - 1) / 2));
        ++y
    ) {
        // clang-format on

        ry = y;
        if (ry < 0) {
            ry = 0;
        } else if (ry > ((int32_t)in_height - 1)) {
            ry = (int32_t)in_height - 1;
        }

        // clang-format off
        for (
            x = (int32_t)x_offset - (((int32_t)win_width - 1) / 2);
            x <= ((int32_t)x_offset + (((int32_t)win_width - 1) / 2));
            ++x
        ) {
            // clang-format on

            rx = x;
            if (rx < 0) {
                rx = 0;
            } else if (rx > ((int32_t)in_width - 1)) {
                rx = (int32_t)in_width - 1;
            }

            out[(oy * win_width) + ox] = in[(ry * in_width) + rx];

            ++ox;
        }
        ++oy;
        ox = 0;
    }
}

double calculate_window_mean(double *img, const uint32_t W, const uint32_t H) {
    double total = 0.0;

    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            total += img[(y * W) + x];
        }
    }

    return total / (double)(W * H);
}

double calculate_window_standard_deviation(
    double *img, const uint32_t W, const uint32_t H, const double window_mean
) {
    double deviation    = 0.0;
    double variance_sum = 0.0;
    double std_dev      = 0.0;

    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            deviation = (img[(y * W) + x] - window_mean);

            variance_sum += deviation * deviation;
        }
    }

    std_dev = sqrt((double)variance_sum / (double)(W * H));

    return std_dev;
}

void zero_mean_window(
    double *img, const uint32_t W, const uint32_t H, const double window_mean
) {
    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            img[(y * W) + x] = img[(y * W) + x] - window_mean;
        }
    }
}

void normalize_window(
    double *img, const uint32_t W, const uint32_t H, const double std_dev
) {
    if (std_dev == 0.0) {
        return;
    }

    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            img[(y * W) + x] = img[(y * W) + x] / std_dev;
        }
    }
}

double window_dot_product(
    double *left, double *right, const uint32_t W, const uint32_t H
) {
    double sum = 0.0;
    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            sum += left[(y * W) + x] * right[(y * W) + x];
        }
    }
    return sum;
}

int32_t find_nearest_nonzero_neighbour(
    int32_t *img, uint32_t W, uint32_t H, uint32_t x, uint32_t y
) {
    if (x >= W || y >= H) {
        return 0;
    }

    if (img[(y * W) + x] != 0) {
        return img[(y * W) + x];
    }

    uint8_t *visited = malloc(sizeof(uint8_t) * W * H);
    memset(visited, 0, sizeof(uint8_t) * W * H);
    coord_fifo_t fifo = {
        .storage  = malloc(sizeof(coord_t) * W * H),
        .read     = 0,
        .write    = 0,
        .capacity = W * H
    };

    visited[(y * W) + x] = 1;
    coord_t *curr        = NULL;
    coord_fifo_enqueue(&fifo, (coord_t){y, x});

#define IN_BOUNDS(coord) \
    (coord.x >= 0 && coord.x < W && coord.y >= 0 && coord.y < H)

    while (coord_fifo_len(&fifo) > 0) {
        curr = coord_fifo_dequeue(&fifo);
        if (curr == NULL) {
            break;
        }
        if (!(IN_BOUNDS((*curr)))) {
            break;
        }
        if (img[(curr->y * W) + curr->x] != 0) {
            break;
        }
        // BFS, look right, down, left, and up if not already visited
        coord_t right = {.x = curr->x + 1, .y = curr->y};
        if (IN_BOUNDS(right) && !visited[(right.y * W) + right.x]) {
            visited[(right.y * W) + right.x] = 1;
            coord_fifo_enqueue(&fifo, right);
        }
        coord_t down = {.x = curr->x, .y = curr->y + 1};
        if (IN_BOUNDS(down) && !visited[(down.y * W) + down.x]) {
            visited[(down.y * W) + down.x] = 1;
            coord_fifo_enqueue(&fifo, down);
        }
        coord_t left = {.x = curr->x - 1, .y = curr->y};
        if (IN_BOUNDS(left) && !visited[(left.y * W) + left.x]) {
            visited[(left.y * W) + left.x] = 1;
            coord_fifo_enqueue(&fifo, left);
        }
        coord_t up = {.x = curr->x, .y = curr->y - 1};
        if (IN_BOUNDS(up) && !visited[(up.y * W) + up.x]) {
            visited[(up.y * W) + up.x] = 1;
            coord_fifo_enqueue(&fifo, up);
        }
    }

    int32_t ret = 0;
    if (curr != NULL) {
        ret = img[(curr->y * W) + curr->x];
    }

    free(visited);
    free(fifo.storage);

    return ret;
}