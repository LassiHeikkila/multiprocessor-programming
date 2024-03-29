#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "image_operations.h"

#include <lodepng.h>

#include "panic.h"

void load_image(const char* path, img_load_result_t* result) {
    if (result == NULL || path == NULL) {
        panic("bad arguments to \"load_image\"");
    }

    result->err = lodepng_decode32_file(
        (uint8_t**)&result->img_desc.img,
        &result->img_desc.width,
        &result->img_desc.height,
        path
    );
}

void output_image(
    const char* path, void* img, palette_e palette, img_write_result_t* result
) {
    if (path == NULL || img == NULL) {
        panic("bad arguments to \"output_image\"");
    }

    unsigned int err = 0;

    switch (palette) {
        case RGBA: {
            rgba_img_t* rgba_img = (rgba_img_t*)img;

            err = lodepng_encode32_file(
                path,
                (uint8_t*)(rgba_img->img),
                rgba_img->width,
                rgba_img->height
            );
            break;
        }
        case GS: {
            gray_img_t* gs_img = (gray_img_t*)img;

            err = lodepng_encode_file(
                path,
                (uint8_t*)(gs_img->img),
                gs_img->width,
                gs_img->height,
                LCT_GREY,
                8
            );
            break;
        }
        case GS_INT32: {
            int32_img_t* i32_img = (int32_img_t*)img;

            gray_t* out =
                malloc(sizeof(gray_t) * i32_img->width * i32_img->height);
            assert(out != NULL);

            for (uint32_t y = 0; y < i32_img->height; ++y) {
                for (uint32_t x = 0; x < i32_img->width; ++x) {
                    out[(y * i32_img->width) + x] =
                        (gray_t)(i32_img->img[(y * i32_img->width) + x] * 255U /
                                 i32_img->max);
                }
            }

            gray_img_t desc = {
                .img = out, .width = i32_img->width, .height = i32_img->height
            };

            output_image(path, &desc, GS, result);
            free(out);
            break;
        }
        case GS_FLOAT: {
            float_img_t* f_img = (float_img_t*)img;

            gray_t* out = malloc(sizeof(gray_t) * f_img->width * f_img->height);
            assert(out != NULL);

            for (uint32_t y = 0; y < f_img->height; ++y) {
                for (uint32_t x = 0; x < f_img->width; ++x) {
                    out[(y * f_img->width) + x] =
                        (gray_t)(f_img->img[(y * f_img->width) + x] * 255.0f /
                                 f_img->max);
                }
            }

            gray_img_t desc = {
                .img = out, .width = f_img->width, .height = f_img->height
            };

            output_image(path, &desc, GS, result);
            free(out);
            break;
        }
        case GS_DOUBLE: {
            double_img_t* d_img = (double_img_t*)img;

            gray_t* out = malloc(sizeof(gray_t) * d_img->width * d_img->height);
            assert(out != NULL);

            for (uint32_t y = 0; y < d_img->height; ++y) {
                for (uint32_t x = 0; x < d_img->width; ++x) {
                    out[(y * d_img->width) + x] =
                        (gray_t)(d_img->img[(y * d_img->width) + x] * 255.0f /
                                 d_img->max);
                }
            }

            gray_img_t desc = {
                .img = out, .width = d_img->width, .height = d_img->height
            };

            output_image(path, &desc, GS, result);
            free(out);
            break;
        }
        default:
            panic("unsupported palette");
    }

    if (result == NULL) {
        return;
    }

    result->err = err;
}

// resizing is done by sampling every nth pixel
void scale_down_image(rgba_img_t* in, rgba_img_t* out) {
    if (in == NULL || out == NULL || out->width == 0 || out->height == 0 ||
        in->height == 0 || in->width == 0) {
        panic("bad arguments to \"scale_down_image\"");
    }
    if (out->width > in->width || out->height > in->height) {
        panic("output dimensions not smaller than input dimensions");
    }

    out->img = malloc(out->width * out->height * (sizeof(rgba_t)));
    if (out->img == NULL) {
        panic("failed to malloc");
    }

    const uint32_t scale_w = in->width / out->width;
    const uint32_t scale_h = in->height / out->height;
    const uint32_t wi      = in->width;
    const uint32_t hi      = in->height;
    const uint32_t wo      = out->width;
    const uint32_t ho      = out->height;

    // tmp variables
    uint32_t xo = 0, xi = 0, yo = 0, yi = 0;

    for (yo = 0; (yo * scale_h) < hi; ++yo) {
        yi = yo * scale_h;

        for (xo = 0; (xo * scale_w) < wi; ++xo) {
            xi = xo * scale_w;

            // cast to uint32_t to copy RGBA in one go
            ((rgba_t*)(out->img))[(yo * wo) + xo] =
                ((rgba_t*)in->img)[(yi * wi) + xi];
        }
    }
}

void convert_to_grayscale(rgba_img_t* in, gray_img_t* out) {
    if (in == NULL || out == NULL || in->height == 0 || in->width == 0) {
        panic("bad arguments to \"convert_to_grayscale\"");
    }

    const uint32_t h = in->height;
    const uint32_t w = in->width;

    out->height = h;
    out->width  = w;
    out->img    = malloc(h * w * sizeof(gray_t));
    if (out->img == NULL) {
        panic("failed to malloc");
    }

    // tmp
    rgba_t rgba;
    double g;

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            rgba = ((rgba_t*)(in->img))[(y * w) + x];
            g    = (SCALE_R * (double)rgba.R) + (SCALE_G * (double)rgba.G) +
                (SCALE_B * (double)rgba.B);

            out->img[(y * w) + x] = (gray_t)(g);
        }
    }
}

void apply_filter(gray_img_t* in, gray_img_t* out) {
    if (in == NULL || out == NULL) {
        panic("bad arguments to \"apply_filter\"");
    }

    const uint32_t h = in->height;
    const uint32_t w = in->width;

    out->height = h;
    out->width  = w;
    out->img    = malloc(h * w * sizeof(gray_t));
    if (out->img == NULL) {
        panic("failed to malloc");
    }

    // variables
    uint32_t y, x;
    int32_t  dy, dx;
    uint32_t py, px;
    // use larger intermediate value to prevent clipping
    uint32_t v;

    // clang-format off
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            v = 0;

            for (dy = -SMOOTHING_KERNEL_RADIUS; dy <= SMOOTHING_KERNEL_RADIUS; ++dy) {
                py = y + dy;

                if (py < 0) {
                    py = 0;
                } else if (py >= h) {
                    py = h - 1;
                };

                for (dx = -SMOOTHING_KERNEL_RADIUS; dx <= SMOOTHING_KERNEL_RADIUS; ++dx) {
                    px = x + dx;

                    if (px < 0) {
                        px = 0;
                    } else if (px >= w) {
                        px = w - 1;
                    }

                    v += in->img[(py * w) + px];
                }
            }

            out->img[(y * w) + x] =
                (gray_t)(v / (SMOOTHING_KERNEL_ORDER * SMOOTHING_KERNEL_ORDER));
        }
    }
    // clang-format on
}

void convert_to_double(gray_img_t* in, double_img_t* out) {
    if (in == NULL || out == NULL || in->height == 0 || in->width == 0) {
        panic("bad arguments to \"convert_to_double\"");
    }

    const uint32_t h = in->height;
    const uint32_t w = in->width;

    out->height = h;
    out->width  = w;
    out->img    = malloc(h * w * sizeof(double));
    if (out->img == NULL) {
        panic("failed to malloc");
    }

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            out->img[(y * w) + x] = (double)(in->img[(y * w) + x]);
        }
    }
}
