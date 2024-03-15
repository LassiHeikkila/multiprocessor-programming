#ifndef _IMAGE_OPERATIONS_H_
#define _IMAGE_OPERATIONS_H_

#include "types.h"

/* Defines */

#define SCALE_R 0.2126
#define SCALE_G 0.7152
#define SCALE_B 0.0722

#define SMOOTHING_KERNEL_ORDER 5
#define SMOOTHING_KERNEL_RADIUS 2

/* Function declarations */

void load_image(const char* path, img_load_result_t* result);

void output_image(
    const char* path, void* img, palette_e palette, img_write_result_t* result
);

void scale_down_image(rgba_img_t* in, rgba_img_t* out);

void convert_to_grayscale(rgba_img_t* in, gray_img_t* out);

void convert_to_float(gray_img_t* in, float_img_t* out);

void apply_filter(gray_img_t* in, gray_img_t* out);

#endif  // _IMAGE_OPERATIONS_H_