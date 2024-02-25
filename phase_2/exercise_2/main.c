#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <lodepng.h>

#include "image_operations.h"
#include "panic.h"
#include "profiling.h"
#include "types.h"

#define IMAGE_ID "0"
#define TEST_IMAGE_PATH "test_images/im" IMAGE_ID ".png"
#define OUTPUT_IMAGE_PATH "output_images/output_" IMAGE_ID ".png"

/* Main function */

int main() {
    PROFILING_BLOCK_DECLARE(resizing);
    PROFILING_BLOCK_DECLARE(grayscaling);
    PROFILING_BLOCK_DECLARE(filtering);

    img_load_result_t load_res = {
        {NULL, 0, 0},
        0
    };
    img_write_result_t output_res = {0};

    puts("hello world!");

    puts("loading image: " TEST_IMAGE_PATH);
    load_image(TEST_IMAGE_PATH, &load_res);
    if (load_res.err) {
        printf(
            "load error %u: %s\n",
            load_res.err,
            lodepng_error_text(load_res.err)
        );
        return 1;
    }

    puts("loaded image successfully!");
    printf("w: %u h: %u\n", load_res.img_desc.width, load_res.img_desc.height);

    puts("scaling image by 0.25x");
    rgba_img_t out = {
        .img    = NULL,
        .width  = load_res.img_desc.width / 4,
        .height = load_res.img_desc.height / 4
    };

    PROFILING_BLOCK_BEGIN(resizing);
    scale_down_image(&load_res.img_desc, &out);
    if (out.img == NULL) {
        puts("scaling failed!");
        return 3;
    }
    PROFILING_BLOCK_END(resizing);

    gray_img_t out_gs = {.width = out.width, .height = out.height};

    puts("converting image to grayscale");

    PROFILING_BLOCK_BEGIN(grayscaling);
    convert_to_grayscale(&out, &out_gs);
    PROFILING_BLOCK_END(grayscaling);

    gray_img_t out_filtered;

    puts("applying filter to image");

    PROFILING_BLOCK_BEGIN(filtering);
    apply_filter(&out_gs, &out_filtered);
    PROFILING_BLOCK_END(filtering);

    puts("outputting image: " OUTPUT_IMAGE_PATH);
    output_image(OUTPUT_IMAGE_PATH, &out_filtered, GS, &output_res);
    if (output_res.err) {
        printf(
            "output error %u: %s\n",
            output_res.err,
            lodepng_error_text(output_res.err)
        );
        return 2;
    }

    free(load_res.img_desc.img);
    free(out.img);
    free(out_gs.img);

    PROFILING_BLOCK_PRINT_US(resizing);
    PROFILING_BLOCK_PRINT_US(grayscaling);
    PROFILING_BLOCK_PRINT_US(filtering);

    return 0;
}
