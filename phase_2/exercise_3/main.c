#include <assert.h>
#include <stdio.h>

#include <CL/cl.h>

#include <lodepng.h>

#include "device_support.h"
#include "image_operations.h"
#include "kernel_wrappers.h"
#include "panic.h"
#include "profiling.h"
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

    const uint32_t Wi = load_res.img_desc.width;
    const uint32_t Hi = load_res.img_desc.height;

    const uint32_t Wo = Wi / 4;
    const uint32_t Ho = Hi / 4;

    printf("input image: %dx%d rgba\n", Wi, Hi);
    printf("output image: %dx%d grayscale\n", Wo, Ho);

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
    assert(queue != NULL);

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

    // Allocate buffers

    cl_mem input_rgba_img = clCreateBuffer(
        ctx,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(rgba_t) * Wi * Hi,
        load_res.img_desc.img,
        &err
    );
    check_cl_error(err);

    cl_mem downscaled_rgba_img = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
        sizeof(rgba_t) * Wo * Ho,
        NULL,
        &err
    );
    check_cl_error(err);

    cl_mem grayscaled_img = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
        sizeof(gray_t) * Wo * Ho,
        NULL,
        &err
    );
    check_cl_error(err);

    cl_mem filtered_img = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
        sizeof(gray_t) * Wo * Ho,
        NULL,
        &err
    );
    check_cl_error(err);

    // work will be split over N*N workers
    // 50 seems to be optimal w.r.t execution time
    const uint32_t N = 50;

    // profiling events
    cl_event downscaling_prof_evt;
    cl_event grayscaling_prof_evt;
    cl_event filtering_prof_evt;

    // enqueue downscaling work
    enqueue_downscaling_work(
        queue,
        downscaling_kernel,
        N,
        Wi,
        Hi,
        Wo,
        Ho,
        input_rgba_img,
        downscaled_rgba_img,
        &downscaling_prof_evt,
        &err
    );
    check_cl_error(err);

    err = clFinish(queue);
    check_cl_error(err);

    // set kernel args for grayscaling
    // enqueue grayscaling work
    enqueue_grayscaling_work(
        queue,
        grayscaling_kernel,
        N,
        Wo,
        Ho,
        downscaled_rgba_img,
        grayscaled_img,
        &grayscaling_prof_evt,
        &err
    );
    check_cl_error(err);

    err = clFinish(queue);
    check_cl_error(err);

    // enqueue filtering work
    enqueue_filtering_work(
        queue,
        filtering_kernel,
        N,
        Wo,
        Ho,
        2,
        grayscaled_img,
        filtered_img,
        &filtering_prof_evt,
        &err
    );
    check_cl_error(err);

    err = clFinish(queue);
    check_cl_error(err);

    gray_t* output_img_data =
        read_device_memory(queue, filtered_img, sizeof(gray_t) * Wo * Ho, &err);
    check_cl_error(err);
    assert(output_img_data != NULL);

    // output image
    gray_img_t output_img = {.img = output_img_data, .width = Wo, .height = Ho};
    img_write_result_t output_res;
    output_image(OUTPUT_IMAGE_PATH, &output_img, GS, &output_res);
    if (output_res.err != 0) {
        printf(
            "failed to output image: %u (\"%s\"\n",
            output_res.err,
            lodepng_error_text(output_res.err)
        );
    }

    // print profiling information
    uint64_t downscaling_ns = get_exec_ns(downscaling_prof_evt);
    uint64_t grayscaling_ns = get_exec_ns(grayscaling_prof_evt);
    uint64_t filtering_ns   = get_exec_ns(filtering_prof_evt);
    PROFILING_RAW_PRINT_US("downscaling", downscaling_ns);
    PROFILING_RAW_PRINT_US("grayscaling", grayscaling_ns);
    PROFILING_RAW_PRINT_US("filtering", filtering_ns);

    // free resources
    free(downscaling_program);
    free(grayscaling_program);
    free(filtering_program);
    free(downscaling_kernel);
    free(grayscaling_kernel);
    free(filtering_kernel);
    free(ctx);
    free(device);
    free(queue);
    free(input_rgba_img);
    free(downscaled_rgba_img);
    free(grayscaled_img);
    free(filtered_img);
    free(output_img_data);
    free(load_res.img_desc.img);
}