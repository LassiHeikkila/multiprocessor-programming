#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>

typedef enum {
    RGBA,  // == red green blue alpha
    GS     // == grayscale
} palette_e;

typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t A;
} rgba_t;

typedef uint8_t gray_t;

typedef struct {
    rgba_t*  img;
    uint32_t width;
    uint32_t height;
} rgba_img_t;

typedef struct {
    gray_t*  img;
    uint32_t width;
    uint32_t height;
} gray_img_t;

typedef struct {
    int32_t* img;
    int32_t  max;
    uint32_t width;
    uint32_t height;
} int32_img_t;

typedef struct {
    double*  img;
    double   max;
    uint32_t width;
    uint32_t height;
} double_img_t;

typedef struct {
    rgba_img_t   img_desc;
    unsigned int err;
} img_load_result_t;

typedef struct {
    unsigned int err;
} img_write_result_t;

#endif  // _TYPES_H_