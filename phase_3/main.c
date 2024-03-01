#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <lodepng.h>

#include "image_operations.h"
#include "panic.h"
#include "types.h"

#define IMAGE_PATH_LEFT "./test_images/im0.png"
#define IMAGE_PATH_RIGHT "./test_images/im1.png"
#define IMAGE_PATH_OUT "./output_images/depthmap.png"

#define WINDOW_WIDTH 9u
#define WINDOW_HEIGHT 9u
#define WINDOW_AREA (WINDOW_WIDTH * WINDOW_HEIGHT)
#define MAX_DISP 260u
#define MAX_GS_VALUE 255u

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
    uint32_t rx = 0;
    uint32_t ry = 0;
    uint32_t ox = 0;
    uint32_t oy = 0;

    // clang-format off
    for (int32_t y = y_offset - ((win_height - 1) / 2); y < (y_offset + ((win_height - 1) / 2)); ++y) {
        // clang-format on

        ry = y;
        if (ry < 0) {
            ry = 0;
        } else if (y > (in_height - 1)) {
            ry = in_height - 1;
        }

        // clang-format off
        for (int32_t x = x_offset - ((win_width - 1) / 2); x < (x_offset + ((win_width - 1) / 2)); ++x) {
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

int32_t elementwise_multiply_windows(
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

int main() {
    // load images from disk
    img_load_result_t img_left;
    img_load_result_t img_right;

    printf("loading images...\n");

    load_image(IMAGE_PATH_LEFT, &img_left);
    if (img_left.err) {
        printf(
            "load error (left image) %u: %s\n",
            img_left.err,
            lodepng_error_text(img_left.err)
        );
        return 1;
    }

    load_image(IMAGE_PATH_RIGHT, &img_right);
    if (img_right.err) {
        printf(
            "load error (right image) %u: %s\n",
            img_right.err,
            lodepng_error_text(img_right.err)
        );
        return 1;
    }

    printf("pre-processing images...\n");

    // downscale
    rgba_img_t img_left_ds = {
        .img    = NULL,
        .width  = img_left.img_desc.width / 4,
        .height = img_left.img_desc.height / 4
    };
    rgba_img_t img_right_ds = {
        .img    = NULL,
        .width  = img_right.img_desc.width / 4,
        .height = img_right.img_desc.height / 4
    };
    scale_down_image(&img_left.img_desc, &img_left_ds);
    scale_down_image(&img_right.img_desc, &img_right_ds);

    // input images no longer needed
    free(img_left.img_desc.img);
    free(img_right.img_desc.img);

    // convert to grayscale
    gray_img_t img_left_gs  = {.img = NULL};
    gray_img_t img_right_gs = {.img = NULL};

    convert_to_grayscale(&img_left_ds, &img_left_gs);
    convert_to_grayscale(&img_right_ds, &img_right_gs);

    // downscaled color images no longer needed
    free(img_left_ds.img);
    free(img_right_ds.img);

    assert(img_left_gs.width == img_right_gs.width);
    assert(img_left_gs.height == img_right_gs.height);

    // ZNCC
    const uint32_t W = img_left_gs.width;
    const uint32_t H = img_left_gs.height;

    int32_t *window_buf_left =
        malloc(sizeof(int32_t) * WINDOW_WIDTH * WINDOW_HEIGHT);
    int32_t *window_buf_right =
        malloc(sizeof(int32_t) * WINDOW_WIDTH * WINDOW_HEIGHT);

    gray_t *disparity_image = malloc(sizeof(gray_t) * W * H);

    printf("computing depthmap:\n");

    double progress = 0.0;

    for (uint32_t y = 0; y < H; ++y) {
        progress = ((double)y / (double)H) * 100.0;
        printf("\rprogress %03.2f%%", progress);
        fflush(stdout);

        for (uint32_t x = 0; x < W; ++x) {
            extract_window(
                img_left_gs.img,
                window_buf_left,
                x,
                y,
                WINDOW_WIDTH,
                WINDOW_HEIGHT,
                W,
                H
            );

            int32_t mean_left = calculate_window_mean(
                window_buf_left, WINDOW_WIDTH, WINDOW_HEIGHT
            );
            int32_t stddev_left = calculate_window_standard_deviation(
                window_buf_left, WINDOW_WIDTH, WINDOW_HEIGHT, mean_left
            );
            zero_mean_window(
                window_buf_left, WINDOW_WIDTH, WINDOW_HEIGHT, mean_left
            );
            normalize_window(
                window_buf_left, WINDOW_WIDTH, WINDOW_HEIGHT, stddev_left
            );

            int32_t max_sum        = 0;
            int32_t best_disparity = 0;

            for (int32_t d = 0; d < MAX_DISP; ++d) {
                if (d > x) {
                    // would go out of bounds
                    break;
                }

                extract_window(
                    img_right_gs.img,
                    window_buf_right,
                    x - d,
                    y,
                    WINDOW_WIDTH,
                    WINDOW_HEIGHT,
                    W,
                    H
                );

                int32_t mean_right = calculate_window_mean(
                    window_buf_right, WINDOW_WIDTH, WINDOW_HEIGHT
                );

                int32_t stddev_right = calculate_window_standard_deviation(
                    window_buf_right, WINDOW_WIDTH, WINDOW_HEIGHT, mean_right
                );

                zero_mean_window(
                    window_buf_right, WINDOW_WIDTH, WINDOW_HEIGHT, mean_right
                );

                normalize_window(
                    window_buf_right, WINDOW_WIDTH, WINDOW_HEIGHT, stddev_right
                );

                int32_t zncc = elementwise_multiply_windows(
                    window_buf_left,
                    window_buf_right,
                    WINDOW_WIDTH,
                    WINDOW_HEIGHT
                );

                if (zncc > max_sum) {
                    max_sum        = zncc;
                    best_disparity = d;
                }
            }

            disparity_image[(y * W) + x] =
                (gray_t)(best_disparity * MAX_GS_VALUE / MAX_DISP);
        }
    }
    printf("\rprogress: 100.00%%\n\n");

    gray_img_t disparity_image_desc = {
        .img = disparity_image, .width = W, .height = H
    };

    printf("outputting depthmap to " IMAGE_PATH_OUT "\n");

    img_write_result_t output_res = {.err = 0};
    output_image(IMAGE_PATH_OUT, &disparity_image_desc, GS, &output_res);
    if (output_res.err != 0) {
        printf(
            "output error %u: %s\n",
            output_res.err,
            lodepng_error_text(output_res.err)
        );
    }

    free(window_buf_left);
    free(window_buf_right);
    free(disparity_image);

    return 0;
}