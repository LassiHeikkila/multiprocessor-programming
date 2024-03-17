#include "zncc_operations.h"
#include "coord_fifo.h"
#include "panic.h"

#include <math.h>
#include <stdbool.h>
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
    int32_t      *img,
    uint32_t      W,
    uint32_t      H,
    uint32_t      x,
    uint32_t      y,
    uint8_t      *visited,
    coord_fifo_t *fifo
) {
    bool         free_visited      = false;
    bool         free_fifo_storage = false;
    coord_fifo_t local_fifo;

    if (x >= W || y >= H) {
        return 0;
    }

    if (img[(y * W) + x] != 0) {
        return img[(y * W) + x];
    }

    if (visited == NULL) {
        visited      = malloc(sizeof(uint8_t) * W * H);
        free_visited = true;
    }
    memset(visited, 0, sizeof(uint8_t) * W * H);
    if (fifo == NULL) {
        free_fifo_storage = true;
        fifo              = &local_fifo;
        fifo->storage     = malloc(sizeof(coord_t) * W * H);
        fifo->capacity    = W * H;
    }
    if (fifo->storage == NULL) {
        panic("NULL fifo->storage!");
    }
    fifo->read  = 0;
    fifo->write = 0;

    visited[(y * W) + x] = 1;
    coord_t *curr        = NULL;
    coord_fifo_enqueue(fifo, (coord_t){.x = x, .y = y});

#define IN_BOUNDS(coord) \
    (coord.x >= 0 && coord.x < W && coord.y >= 0 && coord.y < H)

#define COORD_PTR_TO_IDX(coord) ((coord->y * W) + coord->x)
#define COORD_TO_IDX(coord) ((coord.y * W) + coord.x)

#define VISITED(coord) (visited[(coord.y * W) + coord.x] == 1)

    while (coord_fifo_len(fifo) > 0) {
        curr = coord_fifo_dequeue(fifo);
        if (curr == NULL) {
            break;
        }
        if (!(IN_BOUNDS((*curr)))) {
            break;
        }
        if (img[COORD_PTR_TO_IDX(curr)] != 0) {
            break;
        }
        // BFS, look right, down, left, and up if not already visited
        coord_t right = {.x = curr->x + 1, .y = curr->y};
        if (IN_BOUNDS(right) && !VISITED(right)) {
            visited[COORD_TO_IDX(right)] = 1;
            coord_fifo_enqueue(fifo, right);
        }
        coord_t down = {.x = curr->x, .y = curr->y + 1};
        if (IN_BOUNDS(down) && !VISITED(down)) {
            visited[COORD_TO_IDX(down)] = 1;
            coord_fifo_enqueue(fifo, down);
        }
        coord_t left = {.x = curr->x - 1, .y = curr->y};
        if (IN_BOUNDS(left) && !VISITED(left)) {
            visited[COORD_TO_IDX(left)] = 1;
            coord_fifo_enqueue(fifo, left);
        }
        coord_t up = {.x = curr->x, .y = curr->y - 1};
        if (IN_BOUNDS(up) && !VISITED(up)) {
            visited[COORD_TO_IDX(up)] = 1;
            coord_fifo_enqueue(fifo, up);
        }
    }

    int32_t ret = 0;
    if (curr != NULL) {
        ret = img[(curr->y * W) + curr->x];
    }

    if (free_visited) {
        free(visited);
    }
    if (free_fifo_storage) {
        free(fifo->storage);
    }

    return ret;
}