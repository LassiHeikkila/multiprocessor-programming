#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <lodepng.h>

#include "image_operations.h"
#include "panic.h"

#define IMAGE_PATH_0 "./test_images/im0.png"
#define IMAGE_PATH_1 "./test_images/im1.png"
#define IMAGE_PATH_OUT "./output_images/depthmap.png"

#define WINDOW_SIZE_X 9
#define WINDOW_SIZE_Y 9
#define WINDOW_AREA (WINDOW_SIZE_X * WINDOW_SIZE_Y)
#define MAX_DISP 260

int main() {
    // load images from disk
    img_load_result_t img0;
    img_load_result_t img1;

    load_image(IMAGE_PATH_0, &img0);
    if (img0.err) {
        printf(
            "load error (image 0) %u: %s\n",
            img0.err,
            lodepng_error_text(img0.err)
        );
        return 1;
    }

    load_image(IMAGE_PATH_1, &img1);
    if (img1.err) {
        printf(
            "load error (image 1) %u: %s\n",
            img1.err,
            lodepng_error_text(img1.err)
        );
        return 1;
    }

    // downscale
    rgba_img_t img0_ds = {
        .img    = NULL,
        .width  = img0.img_desc.width / 4,
        .height = img0.img_desc.height / 4
    };
    rgba_img_t img1_ds = {
        .img    = NULL,
        .width  = img1.img_desc.width / 4,
        .height = img1.img_desc.height / 4
    };
    scale_down_image(&img0.img_desc, &img0_ds);
    scale_down_image(&img1.img_desc, &img1_ds);

    // input images no longer needed
    free(img0.img_desc.img);
    free(img1.img_desc.img);

    // convert to grayscale
    gray_img_t img0_gs = {.img = NULL};
    gray_img_t img1_gs = {.img = NULL};

    convert_to_grayscale(&img0_ds, &img0_gs);
    convert_to_grayscale(&img1_ds, &img1_gs);

    // downscaled color images no longer needed
    free(img0_ds.img);
    free(img1_ds.img);

    assert(img0_gs.width == img1_gs.width);
    assert(img0_gs.height == img1_gs.height);

    // ZNCC
    const uint32_t H = img0_gs.height;
    const uint32_t W = img0_gs.width;

    uint32_t *means = malloc(sizeof(uint32_t) * H * W);
    uint32_t *znccs = malloc(sizeof(uint32_t) * H * W);

    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            for (uint32_t d = 0; d < MAX_DISP; ++d) {
                uint32_t total = 0;
                // lets ignore the bottom and right edge for now, if y+wy > H
                // then just use H-1, same for x...
                for (uint32_t wy = 0; wy < WINDOW_SIZE_Y; ++wy) {
                    uint32_t py = y + wy;
                    if (py >= H) {
                        py = H - 1;
                    }

                    for (uint32_t wx = 0; wx < WINDOW_SIZE_X; ++wx) {
                        uint32_t px = x + wx;
                        if (px >= W) {
                            px = W - 1;
                        }

                        total += (uint32_t)(img0_gs.img[(py * W) + px]);
                    }
                }
                uint32_t mean = total / WINDOW_AREA;

                uint32_t zncc = 0;
                for (uint32_t wy = 0; wy < WINDOW_SIZE_Y; ++wy) {
                    uint32_t py = y + wy;
                    if (py >= H) {
                        py = H - 1;
                    }

                    for (uint32_t wx = 0; wx < WINDOW_SIZE_X; ++wx) {
                        uint32_t px = x + wx;
                        if (px >= W) {
                            px = W - 1;
                        }

                        uint32_t zncc;  // how to calculate this?
                    }
                }
            }
        }
    }

    return 0;
}