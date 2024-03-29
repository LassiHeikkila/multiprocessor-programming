#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <lodepng.h>

#include "image_operations.h"
#include "panic.h"
#include "profiling.h"
#include "types.h"
#include "zncc_operations.h"

#define IMAGE_PATH_LEFT "./test_images/im0.png"
#define IMAGE_PATH_RIGHT "./test_images/im1.png"
#define IMAGE_PATH_RAW_OUT_LEFT_TO_RIGHT "./output_images/depthmap_raw_ltr.png"
#define IMAGE_PATH_RAW_OUT_RIGHT_TO_LEFT "./output_images/depthmap_raw_rtl.png"
#define IMAGE_PATH_CROSSCHECKED_OUT "./output_images/depthmap_cc.png"

#define SCALING_FACTOR_WIDTH 4
#define SCALING_FACTOR_HEIGHT 4

#define WINDOW_WIDTH 9u
#define WINDOW_HEIGHT 9u
#define WINDOW_SIZE (WINDOW_WIDTH * WINDOW_HEIGHT)
#define MAX_DISP (260u / SCALING_FACTOR_WIDTH)
#define MAX_GS_VALUE 255u
#define CROSSCHECK_THRESHOLD 8

#define PROGRESS_PRINTS 0

int main() {
    // load images from disk
    img_load_result_t img_left;
    img_load_result_t img_right;

    PROFILING_BLOCK_DECLARE(total_runtime);
    PROFILING_BLOCK_DECLARE(preprocessing);
    PROFILING_BLOCK_DECLARE(zncc_calculation);
    PROFILING_BLOCK_DECLARE(postprocessing);

    PROFILING_BLOCK_BEGIN(total_runtime);

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

    PROFILING_BLOCK_BEGIN(preprocessing);

    // downscale
    rgba_img_t img_left_ds = {
        .img    = NULL,
        .width  = img_left.img_desc.width / SCALING_FACTOR_WIDTH,
        .height = img_left.img_desc.height / SCALING_FACTOR_HEIGHT
    };
    rgba_img_t img_right_ds = {
        .img    = NULL,
        .width  = img_right.img_desc.width / SCALING_FACTOR_WIDTH,
        .height = img_right.img_desc.height / SCALING_FACTOR_HEIGHT
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

    double_img_t img_left_f  = {.img = NULL};
    double_img_t img_right_f = {.img = NULL};

    convert_to_double(&img_left_gs, &img_left_f);
    convert_to_double(&img_right_gs, &img_right_f);

    // don't need GS images anymore
    free(img_left_gs.img);
    free(img_right_gs.img);

    assert(img_left_f.width == img_right_f.width);
    assert(img_left_f.height == img_right_f.height);

    const uint32_t W = img_left_f.width;
    const uint32_t H = img_left_f.height;

    printf("pre-processing data windows...\n");

    const size_t preprocessed_window_size = sizeof(double) * WINDOW_SIZE;
    const size_t windows_count            = W * H;

    // lots of data, around 8GB in total with full size images
    // should not be an issue on typical machines

    double *preprocessed_windows_left =
        malloc(preprocessed_window_size * windows_count);
    double *preprocessed_windows_right =
        malloc(preprocessed_window_size * windows_count);

    assert(preprocessed_windows_left != NULL);
    assert(preprocessed_windows_right != NULL);

    double *mean_left  = malloc(sizeof(double) * W * H);
    double *mean_right = malloc(sizeof(double) * W * H);
    double *std_left   = malloc(sizeof(double) * W * H);
    double *std_right  = malloc(sizeof(double) * W * H);

    for (uint32_t y = 0; y < H; ++y) {
#if PROGRESS_PRINTS == 1
        printf("\rprogress: %03.2f%%", ((double)y / (double)H) * 100.0);
#endif
        for (uint32_t x = 0; x < W; ++x) {
            // left
            {
                double *window =
                    &preprocessed_windows_left[((y * W) + x) * WINDOW_SIZE];

                extract_window(
                    img_left_f.img,
                    window,
                    x,
                    y,
                    WINDOW_WIDTH,
                    WINDOW_HEIGHT,
                    W,
                    H
                );
                double mean =
                    calculate_window_mean(window, WINDOW_WIDTH, WINDOW_HEIGHT);
                double stddev = calculate_window_standard_deviation(
                    window, WINDOW_WIDTH, WINDOW_HEIGHT, mean
                );
                zero_mean_window(window, WINDOW_WIDTH, WINDOW_HEIGHT, mean);
                normalize_window(window, WINDOW_WIDTH, WINDOW_HEIGHT, stddev);

                mean_left[(y * W) + x] = mean;
                std_left[(y * W) + x]  = stddev;
            }

            // right
            {
                double *window =
                    &preprocessed_windows_right[((y * W) + x) * WINDOW_SIZE];
                extract_window(
                    img_right_f.img,
                    window,
                    x,
                    y,
                    WINDOW_WIDTH,
                    WINDOW_HEIGHT,
                    W,
                    H
                );
                double mean =
                    calculate_window_mean(window, WINDOW_WIDTH, WINDOW_HEIGHT);
                double stddev = calculate_window_standard_deviation(
                    window, WINDOW_WIDTH, WINDOW_HEIGHT, mean
                );
                zero_mean_window(window, WINDOW_WIDTH, WINDOW_HEIGHT, mean);
                normalize_window(window, WINDOW_WIDTH, WINDOW_HEIGHT, stddev);

                mean_right[(y * W) + x] = mean;
                std_right[(y * W) + x]  = stddev;
            }
        }
    }
#if PROGRESS_PRINTS == 1
    printf("\rprogress: 100.00%%\n\n");
#endif
    PROFILING_BLOCK_END(preprocessing);

    {
        double_img_t tmp_mean_l = {
            .img    = mean_left,
            .max    = (double)MAX_GS_VALUE,
            .width  = W,
            .height = H
        };
        output_image(
            "./output_images/mean_left.png", &tmp_mean_l, GS_DOUBLE, NULL
        );
    }
    {
        double_img_t tmp_mean_r = {
            .img    = mean_right,
            .max    = (double)MAX_GS_VALUE,
            .width  = W,
            .height = H
        };
        output_image(
            "./output_images/mean_right.png", &tmp_mean_r, GS_DOUBLE, NULL
        );
    }
    {
        double_img_t tmp_stddev_l = {
            .img    = std_left,
            .max    = (double)MAX_GS_VALUE,
            .width  = W,
            .height = H
        };
        output_image(
            "./output_images/std_left.png", &tmp_stddev_l, GS_DOUBLE, NULL
        );
    }
    {
        double_img_t tmp_stddev_r = {
            .img    = std_right,
            .max    = (double)MAX_GS_VALUE,
            .width  = W,
            .height = H
        };
        output_image(
            "./output_images/std_right.png", &tmp_stddev_r, GS_DOUBLE, NULL
        );
    }

    // don't need float input images anymore
    free(img_left_f.img);
    free(img_right_f.img);

    // don't need intermediate images either
    free(mean_left);
    free(mean_right);
    free(std_left);
    free(std_right);

    // ZNCC
    PROFILING_BLOCK_BEGIN(zncc_calculation);

    int32_t *disparity_image_left = malloc(sizeof(int32_t) * W * H);
    assert(disparity_image_left != NULL);
    memset(disparity_image_left, 0, sizeof(int32_t) * W * H);

    int32_t *disparity_image_right = malloc(sizeof(int32_t) * W * H);
    assert(disparity_image_right != NULL);
    memset(disparity_image_right, 0, sizeof(int32_t) * W * H);

    printf("computing depthmap left to right:\n");
    // LEFT to RIGHT
    for (uint32_t y = 0; y < H; ++y) {
#if PROGRESS_PRINTS == 1
        printf("\rprogress: %03.2f%%", ((double)y / (double)H) * 100.0);
        fflush(stdout);
#endif

        for (uint32_t x = 0; x < W; ++x) {
            double  max_sum        = 0;
            int32_t best_disparity = 0;
            double *window_left =
                &preprocessed_windows_left[((y * W) + x) * WINDOW_SIZE];

            for (int32_t d = 0; d < MAX_DISP; ++d) {
                if (d > x) {
                    // would go out of bounds
                    break;
                }

                double *window_right = &preprocessed_windows_right
                                           [((y * W) + (x - d)) * WINDOW_SIZE];

                double zncc = window_dot_product(
                    window_left, window_right, WINDOW_WIDTH, WINDOW_HEIGHT
                );

                if (zncc > max_sum) {
                    max_sum        = zncc;
                    best_disparity = d;
                }
            }

            disparity_image_left[(y * W) + x] = best_disparity;
        }
    }
#if PROGRESS_PRINTS == 1
    printf("\rprogress: 100.00%%\n\n");
#endif
    printf("computing depthmap right to left:\n");
    // RIGHT to LEFT
    for (uint32_t y = 0; y < H; ++y) {
#if PROGRESS_PRINTS == 1
        printf("\rprogress: %03.2f%%", ((double)y / (double)H) * 100.0);
        fflush(stdout);
#endif
        for (uint32_t x = 0; x < W; ++x) {
            double  max_sum        = 0;
            int32_t best_disparity = 0;

            double *window_right =
                &preprocessed_windows_right[((y * W) + x) * WINDOW_SIZE];

            for (int32_t d = 0; d < MAX_DISP; ++d) {
                if (d > (W - x - 1)) {
                    // would go out of bounds
                    break;
                }

                double *window_left = &preprocessed_windows_left
                                          [((y * W) + (x + d)) * WINDOW_SIZE];

                double zncc = window_dot_product(
                    window_left, window_right, WINDOW_WIDTH, WINDOW_HEIGHT
                );

                if (zncc > max_sum) {
                    max_sum        = zncc;
                    best_disparity = d;
                }
            }

            disparity_image_right[(y * W) + x] = best_disparity;
        }
    }
#if PROGRESS_PRINTS == 1
    printf("\rprogress: 100.00%%\n\n");
#endif

    PROFILING_BLOCK_END(zncc_calculation);

    printf("outputting raw depthmaps\n");

    {
        int32_img_t disp_img_left = {
            .img    = disparity_image_left,
            .max    = MAX_DISP,
            .width  = W,
            .height = H
        };
        output_image(
            IMAGE_PATH_RAW_OUT_LEFT_TO_RIGHT, &disp_img_left, GS_INT32, NULL
        );
    }
    {
        int32_img_t disp_img_right = {
            .img    = disparity_image_right,
            .max    = MAX_DISP,
            .width  = W,
            .height = H
        };
        output_image(
            IMAGE_PATH_RAW_OUT_RIGHT_TO_LEFT, &disp_img_right, GS_INT32, NULL
        );
    }

    int32_t *combined = malloc(sizeof(int32_t) * W * H);
    assert(combined != NULL);

    PROFILING_BLOCK_BEGIN(postprocessing);

    printf("cross-checking...\n");
    for (uint32_t y = 0; y < H; ++y) {
#if PROGRESS_PRINTS == 1
        printf("\rprogress: %03.2f%%", ((double)y / (double)H) * 100.0);
        fflush(stdout);
#endif
        for (uint32_t x = 0; x < W; ++x) {
            int32_t disparity = disparity_image_left[(y * W) + x];

            int32_t delta = disparity_image_left[(y * W) + (x)] -
                            disparity_image_right[(y * W) + (x - disparity)];

            if (delta < 0) {
                delta = -delta;
            }

            combined[(y * W) + x] =
                (delta <= CROSSCHECK_THRESHOLD
                     ? disparity_image_left[(y * W) + x]
                     : 0);
        }
    }
#if PROGRESS_PRINTS == 1
    printf("\rprogress: 100.00%%\n\n");
#endif

    printf("filling empty regions...\n");
    uint8_t     *visited = malloc(W * H * sizeof(uint8_t));
    coord_fifo_t fifo    = {
           .storage  = malloc(W * H * sizeof(coord_t)),
           .read     = 0,
           .write    = 0,
           .capacity = W * H
    };
    for (uint32_t y = 0; y < H; ++y) {
#if PROGRESS_PRINTS == 1
        printf("\rprogress: %03.2f%%", ((double)y / (double)H) * 100.0);
        fflush(stdout);
#endif

        for (uint32_t x = 0; x < W; ++x) {
            int32_t curr = combined[(y * W) + x];
            if (curr == 0) {
                int32_t nnzn = find_nearest_nonzero_neighbour(
                    combined, W, H, x, y, visited, &fifo
                );
                combined[(y * W) + x] = nnzn;
            }
        }
    }
    free(fifo.storage);
    free(visited);
#if PROGRESS_PRINTS == 1
    printf("\rprogress: 100.00%%\n\n");
#endif

    PROFILING_BLOCK_END(postprocessing);

    printf("output crosschecked depthmap\n");

    int32_img_t combined_img = {
        .img = combined, .max = MAX_DISP, .width = W, .height = H
    };
    output_image(IMAGE_PATH_CROSSCHECKED_OUT, &combined_img, GS_INT32, NULL);

    free(preprocessed_windows_left);
    free(preprocessed_windows_right);
    free(disparity_image_left);
    free(disparity_image_right);
    free(combined);

    PROFILING_BLOCK_END(total_runtime);

    PROFILING_BLOCK_PRINT_MS(preprocessing);
    PROFILING_BLOCK_PRINT_S(zncc_calculation);
    PROFILING_BLOCK_PRINT_MS(postprocessing);
    PROFILING_BLOCK_PRINT_S(total_runtime);

    return 0;
}
