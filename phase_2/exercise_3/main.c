#include <stdio.h>

#include <CL/cl.h>

#include <lodepng.h>

#include "device_support.h"
#include "image_operations.h"
#include "types.h"

#define IMAGE_ID "0"
#define TEST_IMAGE_PATH "../test_images/im" IMAGE_ID ".png"
#define OUTPUT_IMAGE_PATH "output_images/output_" IMAGE_ID ".png"

#define DOWNSCALING_KERNEL_SRC_PATH "downscaling_kernel.cl"
#define GRAYSCALING_KERNEL_SRC_PATH "grayscaling_kernel.cl"
#define FILTERING_KERNEL_SRC_PATH "smoothing_kernel.cl"

#define DOWNSCALING_KERNEL_NAME "downscale_kernel"
#define GRAYSCALING_KERNEL_NAME "grayscale_kernel"
#define FILTERING_KERNEL_NAME "smoothing_kernel"

int main() {
    puts("hello world");

    // load image
    img_load_result_t load_res = {
        {NULL, 0, 0},
        0
    };
    img_write_result_t output_res = {0};

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

    // compile kernels
    // - resizing
    // - grayscaling
    // - filtering
    cl_program       downscaling_program;
    cl_program       grayscaling_program;
    cl_program       filtering_program;
    cl_kernel        downscaling_kernel;
    cl_kernel        grayscaling_kernel;
    cl_kernel        filtering_kernel;
    cl_context       ctx;
    cl_device_id     device;
    cl_command_queue queue;
    cl_int           err;

    device = get_device(&err);
    check_cl_error(err);

    ctx = create_context(device, &err);
    check_cl_error(err);

    queue = create_queue(ctx, device, &err);
    check_cl_error(err);

    downscaling_program =
        compile_program_from_file(DOWNSCALING_KERNEL_SRC_PATH, ctx, &err);
    check_cl_error(err);
    downscaling_kernel =
        build_kernel(DOWNSCALING_KERNEL_NAME, downscaling_program, &err);
    check_cl_error(err);

    grayscaling_program =
        compile_program_from_file(GRAYSCALING_KERNEL_SRC_PATH, ctx, &err);
    check_cl_error(err);
    grayscaling_kernel =
        build_kernel(GRAYSCALING_KERNEL_NAME, grayscaling_program, &err);
    check_cl_error(err);

    filtering_program =
        compile_program_from_file(FILTERING_KERNEL_SRC_PATH, ctx, &err);
    check_cl_error(err);
    filtering_kernel =
        build_kernel(FILTERING_KERNEL_NAME, filtering_program, &err);
    check_cl_error(err);

    // run kernels
    // note about work dimensions:
    // first dimension is for x, second dimension for y

    // output image

    // print profiling information
}