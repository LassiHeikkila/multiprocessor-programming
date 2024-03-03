#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <lodepng.h>

#include "image_operations.h"
#include "panic.h"
#include "types.h"
#include "zncc_operations.h"

#define IMAGE_PATH_LEFT "./test_images/im0.png"
#define IMAGE_PATH_RIGHT "./test_images/im1.png"
#define IMAGE_PATH_OUT "./output_images/depthmap.png"

#define WINDOW_WIDTH 9u
#define WINDOW_HEIGHT 9u
#define MAX_DISP 260u
#define MAX_GS_VALUE 255u

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

    // don't need original images anymore
    free(img_left.img_desc.img);
    free(img_right.img_desc.img);

    // convert to grayscale
    gray_img_t img_left_gs  = {.img = NULL};
    gray_img_t img_right_gs = {.img = NULL};

    convert_to_grayscale(&img_left_ds, &img_left_gs);
    convert_to_grayscale(&img_right_ds, &img_right_gs);

    // don't need scaled down RGBA images anymore
    free(img_left_ds.img);
    free(img_right_ds.img);

    assert(img_left_gs.width == img_right_gs.width);
    assert(img_left_gs.height == img_right_gs.height);

    // ZNCC
    const uint32_t W = img_left_gs.width;
    const uint32_t H = img_left_gs.height;

    int32_t *window_buf_left =
        malloc(sizeof(int32_t) * WINDOW_WIDTH * WINDOW_HEIGHT);
    assert(window_buf_left != NULL);

    int32_t *window_buf_right =
        malloc(sizeof(int32_t) * WINDOW_WIDTH * WINDOW_HEIGHT);
    assert(window_buf_right != NULL);

    gray_t *disparity_image = malloc(sizeof(gray_t) * W * H);
    assert(disparity_image != NULL);
    memset(disparity_image, 0, sizeof(gray_t) * W * H);

    printf("computing depthmap:\n");

    uint32_t x, y, d;
    int32_t  max_sum;
    int32_t  best_disparity;
    int32_t  mean_left;
    int32_t  stddev_left;
    int32_t  mean_right;
    int32_t  stddev_right;
    int32_t  zncc;

    for (y = 0; y < H; ++y) {
        printf("\rprogress %03.2f%%", ((double)y / (double)H) * 100.0);
        fflush(stdout);

        for (x = 0; x < W; ++x) {
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

            mean_left = calculate_window_mean(
                window_buf_left, WINDOW_WIDTH, WINDOW_HEIGHT
            );
            stddev_left = calculate_window_standard_deviation(
                window_buf_left, WINDOW_WIDTH, WINDOW_HEIGHT, mean_left
            );
            zero_mean_window(
                window_buf_left, WINDOW_WIDTH, WINDOW_HEIGHT, mean_left
            );
            normalize_window(
                window_buf_left, WINDOW_WIDTH, WINDOW_HEIGHT, stddev_left
            );

            max_sum        = 0;
            best_disparity = 0;

            for (d = 0; d < MAX_DISP; ++d) {
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

                mean_right = calculate_window_mean(
                    window_buf_right, WINDOW_WIDTH, WINDOW_HEIGHT
                );

                stddev_right = calculate_window_standard_deviation(
                    window_buf_right, WINDOW_WIDTH, WINDOW_HEIGHT, mean_right
                );

                zero_mean_window(
                    window_buf_right, WINDOW_WIDTH, WINDOW_HEIGHT, mean_right
                );

                normalize_window(
                    window_buf_right, WINDOW_WIDTH, WINDOW_HEIGHT, stddev_right
                );

                zncc = sum_of_elementwise_multiply_windows(
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

    // don't need GS input images anymore
    free(img_left_gs.img);
    free(img_right_gs.img);

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