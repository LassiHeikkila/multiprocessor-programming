#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <CL/cl.h>

#include "device_support.h"
#include "image_operations.h"
#include "profiling.h"
#include "types.h"

#define IMAGE_PATH_LEFT "./test_images/im0.png"
#define IMAGE_PATH_RIGHT "./test_images/im1.png"
#define IMAGE_PATH_OUT "./output_images/depthmap_cc.png"

#define DOWNSCALING_KERNEL_FILE "./kernels/resize.cl"
#define DOWNSCALING_KERNEL_NAME "downscale_kernel"

#define GRAYSCALING_KERNEL_FILE "./kernels/grayscale.cl"
#define GRAYSCALING_KERNEL_NAME "grayscale_kernel"

#define ZNCC_KERNEL_FILE "./kernels/zncc.cl"
#define ZNCC_EXTRACT_DATA_WINDOWS_NAME "extract_data_windows"
#define ZNCC_CALCULATE_NAME "calculate_zncc"
#define ZNCC_CROSS_CHECK_NAME "cross_check"
#define ZNCC_FILL_ZEROS_NAME "fill_zero_regions"

#define DOWNSCALING_FACTOR_W 4
#define DOWNSCALING_FACTOR_H 4

#define NUM_ROWS 504  // probably should be calculated

#define MAX_DISP (260u / DOWNSCALING_FACTOR_W)
#define MAX_GS_VALUE 255u
#define CROSSCHECK_THRESHOLD 8

#define err_check(e) check_cl_error_with_file_line(__FILE__, __LINE__, e)

void enqueue_downscaling_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   Wi,
    const uint32_t   Hi,
    const uint32_t   Wo,
    const uint32_t   Ho,
    cl_mem           img_in,
    cl_mem           img_out,
    cl_event        *profiling_evt,
    cl_int          *err
);

void enqueue_grayscaling_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   W,
    const uint32_t   H,
    cl_mem           img_in,
    cl_mem           img_out,
    cl_event        *profiling_evt,
    cl_int          *err
);

void enqueue_zncc_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   M,
    const uint32_t   W,
    const uint32_t   H,
    const int32_t    direction,
    const uint32_t   max_disparity,
    cl_mem           img_left,
    cl_mem           img_right,
    cl_mem           disp_img_out,
    cl_event        *profiling_evt,
    cl_int          *err
);

void enqueue_cross_check_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   W,
    const uint32_t   H,
    cl_mem           disp_img_left,
    cl_mem           disp_img_right,
    cl_mem           out,
    cl_event        *profiling_evt,
    cl_int          *err
);

void enqueue_zero_fill_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   W,
    const uint32_t   H,
    cl_mem           img,
    cl_event        *profiling_evt,
    cl_int          *err
);

int main() {
    PROFILING_BLOCK_DECLARE(total_runtime);
    PROFILING_BLOCK_DECLARE(preprocessing);
    PROFILING_BLOCK_DECLARE(zncc_calculation);
    PROFILING_BLOCK_DECLARE(postprocessing);

    PROFILING_BLOCK_BEGIN(total_runtime);
    PROFILING_BLOCK_BEGIN(preprocessing);

    // setup OpenCL environment
    cl_int           err           = CL_SUCCESS;
    cl_device_id     dev           = NULL;
    cl_context       ctx           = NULL;
    cl_command_queue queue         = NULL;
    cl_program       downscaling_p = NULL;
    cl_program       grayscaling_p = NULL;
    cl_kernel        downscaling_k = NULL;
    cl_kernel        grayscaling_k = NULL;
    cl_program       zncc_p        = NULL;
    cl_kernel        zncc_k        = NULL;
    cl_kernel        cross_check_k = NULL;
    cl_kernel        zero_fill_k   = NULL;

    printf("set up OpenCL runtime...\n");

    dev = get_device(&err);
    check_cl_error(err);

    ctx = create_context(dev, &err);
    check_cl_error(err);

    queue = create_queue(ctx, dev, &err);
    check_cl_error(err);

    printf("build OpenCL kernels...\n");

    downscaling_p =
        compile_program_from_file(DOWNSCALING_KERNEL_FILE, ctx, dev, &err);
    err_check(err);

    downscaling_k = build_kernel(DOWNSCALING_KERNEL_NAME, downscaling_p, &err);
    err_check(err);

    grayscaling_p =
        compile_program_from_file(GRAYSCALING_KERNEL_FILE, ctx, dev, &err);
    err_check(err);

    grayscaling_k = build_kernel(GRAYSCALING_KERNEL_NAME, grayscaling_p, &err);
    err_check(err);

    zncc_p = compile_program_from_file(ZNCC_KERNEL_FILE, ctx, dev, &err);
    err_check(err);

    zncc_k = build_kernel(ZNCC_CALCULATE_NAME, zncc_p, &err);
    err_check(err);

    cross_check_k = build_kernel(ZNCC_CROSS_CHECK_NAME, zncc_p, &err);
    err_check(err);

    zero_fill_k = build_kernel(ZNCC_FILL_ZEROS_NAME, zncc_p, &err);
    err_check(err);

    print_device_info(dev);

    // load images
    printf("loading input images into memory...\n");
    img_load_result_t load_left;
    img_load_result_t load_right;
    load_image(IMAGE_PATH_LEFT, &load_left);
    load_image(IMAGE_PATH_RIGHT, &load_right);

    assert(load_left.img_desc.img != NULL);
    assert(load_right.img_desc.img != NULL);
    assert(load_left.img_desc.width == load_right.img_desc.width);
    assert(load_left.img_desc.height == load_right.img_desc.height);

    const uint32_t W_orig = load_left.img_desc.width;
    const uint32_t H_orig = load_left.img_desc.height;
    const uint32_t W_ds   = W_orig / DOWNSCALING_FACTOR_W;
    const uint32_t H_ds   = H_orig / DOWNSCALING_FACTOR_H;

    // create buffers for images and load original images into device memory
    cl_mem dev_image_left, dev_image_right;        // originals
    cl_mem dev_image_ds_left, dev_image_ds_right;  // downscaled
    cl_mem dev_image_gs_left, dev_image_gs_right;  // grayscale

    const size_t          row_pitch_original = sizeof(rgba_t) * W_orig;
    const size_t          row_pitch_ds       = sizeof(rgba_t) * W_ds;
    const size_t          row_pitch_gs       = sizeof(float) * W_ds;
    const cl_image_format image_format_rgba  = {CL_RGBA, CL_UNSIGNED_INT8};
    const cl_image_format image_format_gs    = {CL_R, CL_FLOAT};

    dev_image_left = clCreateImage2D(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        &image_format_rgba,
        (size_t)W_orig,
        (size_t)H_orig,
        row_pitch_original,
        load_left.img_desc.img,
        &err
    );
    err_check(err);

    dev_image_right = clCreateImage2D(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        &image_format_rgba,
        (size_t)W_orig,
        (size_t)H_orig,
        row_pitch_original,
        load_right.img_desc.img,
        &err
    );
    err_check(err);

    dev_image_ds_left = clCreateImage2D(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
        &image_format_rgba,
        (size_t)W_ds,
        (size_t)H_ds,
        row_pitch_ds,
        NULL,
        &err
    );
    err_check(err);

    dev_image_ds_right = clCreateImage2D(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
        &image_format_rgba,
        (size_t)W_ds,
        (size_t)H_ds,
        row_pitch_ds,
        NULL,
        &err
    );
    err_check(err);

    dev_image_gs_left = clCreateImage2D(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
        &image_format_gs,
        (size_t)W_ds,
        (size_t)H_ds,
        row_pitch_gs,
        NULL,
        &err
    );
    err_check(err);

    dev_image_gs_right = clCreateImage2D(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
        &image_format_gs,
        (size_t)W_ds,
        (size_t)H_ds,
        row_pitch_gs,
        NULL,
        &err
    );
    err_check(err);

    // execute downscaling for left and right images
    printf("downscaling input images...\n");
    cl_event prof_evt_ds_left  = NULL;
    cl_event prof_evt_ds_right = NULL;

    enqueue_downscaling_work(
        queue,
        downscaling_k,
        NUM_ROWS,
        W_orig,
        H_orig,
        W_ds,
        H_ds,
        dev_image_left,
        dev_image_ds_left,
        &prof_evt_ds_left,
        &err
    );
    err_check(err);

    enqueue_downscaling_work(
        queue,
        downscaling_k,
        NUM_ROWS,
        W_orig,
        H_orig,
        W_ds,
        H_ds,
        dev_image_right,
        dev_image_ds_right,
        &prof_evt_ds_right,
        &err
    );
    err_check(err);

    err = clFinish(queue);
    err_check(err);

    // execute grayscaling work for left and right images
    printf("grayscaling input images...\n");
    cl_event prof_evt_gs_left  = NULL;
    cl_event prof_evt_gs_right = NULL;

    enqueue_grayscaling_work(
        queue,
        grayscaling_k,
        NUM_ROWS,
        W_ds,
        H_ds,
        dev_image_ds_left,
        dev_image_gs_left,
        &prof_evt_gs_left,
        &err
    );
    err_check(err);

    enqueue_grayscaling_work(
        queue,
        grayscaling_k,
        NUM_ROWS,
        W_ds,
        H_ds,
        dev_image_ds_right,
        dev_image_gs_right,
        &prof_evt_gs_right,
        &err
    );
    err_check(err);

    err = clFinish(queue);
    err_check(err);

    // free original and downscaled color images
    clReleaseMemObject(dev_image_left);
    clReleaseMemObject(dev_image_right);
    clReleaseMemObject(dev_image_ds_left);
    clReleaseMemObject(dev_image_ds_right);

    PROFILING_BLOCK_END(preprocessing);

    // do ZNCC calculations
    PROFILING_BLOCK_BEGIN(zncc_calculation);

    printf("calculate zncc...\n");
    cl_event prof_evt_zncc_l = NULL;
    cl_event prof_evt_zncc_r = NULL;
    cl_mem   dev_disp_left   = NULL;
    cl_mem   dev_disp_right  = NULL;

    dev_disp_left = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
        W_ds * H_ds * sizeof(int32_t),
        NULL,
        &err
    );
    err_check(err);

    dev_disp_right = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
        W_ds * H_ds * sizeof(int32_t),
        NULL,
        &err
    );
    err_check(err);

#define ZNCC_W_SEGMENTS 1
#define ZNCC_H_SEGMENTS NUM_ROWS

    enqueue_zncc_work(
        queue,
        zncc_k,
        ZNCC_H_SEGMENTS,
        ZNCC_W_SEGMENTS,
        W_ds,
        H_ds,
        1,
        MAX_DISP,
        dev_image_gs_left,
        dev_image_gs_right,
        dev_disp_left,
        &prof_evt_zncc_l,
        &err
    );
    err_check(err);

    enqueue_zncc_work(
        queue,
        zncc_k,
        ZNCC_H_SEGMENTS,
        ZNCC_W_SEGMENTS,
        W_ds,
        H_ds,
        -1,
        MAX_DISP,
        dev_image_gs_left,
        dev_image_gs_right,
        dev_disp_right,
        &prof_evt_zncc_r,
        &err
    );
    err_check(err);

    err = clFinish(queue);
    err_check(err);

    PROFILING_BLOCK_END(zncc_calculation);

    // free images which are no longer needed
    clReleaseMemObject(dev_image_gs_left);
    clReleaseMemObject(dev_image_gs_right);

    // postprocessing
    PROFILING_BLOCK_BEGIN(postprocessing);
    printf("postprocessing disparity data...\n");

    cl_event prof_evt_cross_check = NULL;
    cl_event prof_evt_zero_fill   = NULL;
    cl_mem   dev_combined_image   = NULL;

    dev_combined_image = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
        W_ds * H_ds * sizeof(int32_t),
        NULL,
        &err
    );
    err_check(err);

    enqueue_cross_check_work(
        queue,
        cross_check_k,
        NUM_ROWS,
        W_ds,
        H_ds,
        dev_disp_left,
        dev_disp_right,
        dev_combined_image,
        &prof_evt_cross_check,
        &err
    );
    err_check(err);

    err = clFinish(queue);
    err_check(err);

    enqueue_zero_fill_work(
        queue,
        zero_fill_k,
        1,
        W_ds,
        H_ds,
        dev_combined_image,
        &prof_evt_zero_fill,
        &err
    );
    err_check(err);

    err = clFinish(queue);
    err_check(err);

    PROFILING_BLOCK_END(postprocessing);
    PROFILING_BLOCK_END(total_runtime);

    int32_img_t depthmap = {
        .img    = malloc(W_ds * H_ds * sizeof(int32_t)),
        .max    = MAX_DISP,
        .width  = W_ds,
        .height = H_ds
    };
    memset(depthmap.img, 1, W_ds * H_ds * sizeof(int32_t));

    err = clEnqueueReadBuffer(
        queue,
        dev_combined_image,
        CL_TRUE,
        0,
        W_ds * H_ds * sizeof(int32_t),
        (void *)depthmap.img,
        0,
        NULL,
        NULL
    );
    err_check(err);

    err = clFinish(queue);
    err_check(err);

    img_write_result_t r = {.err = 0};
    output_grayscale_int32_image(
        IMAGE_PATH_OUT,
        depthmap.img,
        depthmap.width,
        depthmap.height,
        depthmap.max,
        &r
    );
    if (r.err != 0) {
        printf("error outputting depthmap: %d\n", r.err);
    }

    // free remaining resources
    clReleaseMemObject(dev_disp_left);
    clReleaseMemObject(dev_disp_right);
    clReleaseMemObject(dev_combined_image);
    free(depthmap.img);

    // print profiling information

    uint64_t ds_ns =
        get_exec_ns(prof_evt_ds_left) + get_exec_ns(prof_evt_ds_right);
    uint64_t gs_ns =
        get_exec_ns(prof_evt_gs_left) + get_exec_ns(prof_evt_gs_right);
    uint64_t zncc_ns =
        get_exec_ns(prof_evt_zncc_l) + get_exec_ns(prof_evt_zncc_r);
    uint64_t postprocess_ns =
        get_exec_ns(prof_evt_cross_check) + get_exec_ns(prof_evt_zero_fill);

    printf("\nOpenCL profiling blocks:\n");
    PROFILING_RAW_PRINT_US("downscaling", ds_ns);
    PROFILING_RAW_PRINT_US("grayscaling", gs_ns);
    PROFILING_RAW_PRINT_MS("zncc_calculation", zncc_ns);
    PROFILING_RAW_PRINT_US("postprocessing", postprocess_ns);

    printf("\nhost program profiling blocks:\n");
    PROFILING_BLOCK_PRINT_MS(preprocessing);
    PROFILING_BLOCK_PRINT_S(zncc_calculation);
    PROFILING_BLOCK_PRINT_MS(postprocessing);
    PROFILING_BLOCK_PRINT_S(total_runtime);

    return 0;
}

#define SET_KERNEL_ARG(idx, t, v)                                 \
    internal_err = clSetKernelArg(kernel, (idx), sizeof(t), (v)); \
    if (internal_err != CL_SUCCESS) {                             \
        printf("error setting argument %d\n", (idx));             \
        *err = internal_err;                                      \
        return;                                                   \
    }

void enqueue_grayscaling_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   Wi,
    const uint32_t   Hi,
    cl_mem           img_in,
    cl_mem           img_out,
    cl_event        *profiling_evt,
    cl_int          *err
) {
    cl_int internal_err;
    if (err == NULL) {
        err = &internal_err;
    }

    SET_KERNEL_ARG(0, uint32_t, &N)
    SET_KERNEL_ARG(1, uint32_t, &Wi)
    SET_KERNEL_ARG(2, uint32_t, &Hi)
    SET_KERNEL_ARG(3, cl_mem, &img_in)
    SET_KERNEL_ARG(4, cl_mem, &img_out)

    const size_t global_id = N;

    internal_err = clEnqueueNDRangeKernel(
        queue, kernel, 1, NULL, &global_id, NULL, 0, NULL, profiling_evt
    );
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    *err = CL_SUCCESS;
}

void enqueue_downscaling_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   Wi,
    const uint32_t   Hi,
    const uint32_t   Wo,
    const uint32_t   Ho,
    cl_mem           img_in,
    cl_mem           img_out,
    cl_event        *profiling_evt,
    cl_int          *err
) {
    cl_int internal_err;
    if (err == NULL) {
        err = &internal_err;
    }

    SET_KERNEL_ARG(0, uint32_t, &N)
    SET_KERNEL_ARG(1, uint32_t, &Wi)
    SET_KERNEL_ARG(2, uint32_t, &Hi)
    SET_KERNEL_ARG(3, uint32_t, &Wo)
    SET_KERNEL_ARG(4, uint32_t, &Ho)
    SET_KERNEL_ARG(5, cl_mem, &img_in)
    SET_KERNEL_ARG(6, cl_mem, &img_out)

    const size_t global_id = N;

    internal_err = clEnqueueNDRangeKernel(
        queue, kernel, 1, NULL, &global_id, NULL, 0, NULL, profiling_evt
    );
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    *err = CL_SUCCESS;
}

void enqueue_zncc_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   M,
    const uint32_t   W_i,
    const uint32_t   H_i,
    const int32_t    direction,
    const uint32_t   max_disparity,
    cl_mem           img_left,
    cl_mem           img_right,
    cl_mem           disp_img_out,
    cl_event        *profiling_evt,
    cl_int          *err
) {
    cl_int internal_err;
    if (err == NULL) {
        err = &internal_err;
    }

    SET_KERNEL_ARG(0, uint32_t, &N);
    SET_KERNEL_ARG(1, uint32_t, &M);
    SET_KERNEL_ARG(2, uint32_t, &W_i);
    SET_KERNEL_ARG(3, uint32_t, &H_i);
    SET_KERNEL_ARG(4, int32_t, &direction);
    SET_KERNEL_ARG(5, uint32_t, &max_disparity);
    SET_KERNEL_ARG(6, cl_mem, &img_left);
    SET_KERNEL_ARG(7, cl_mem, &img_right);
    SET_KERNEL_ARG(8, cl_mem, &disp_img_out);

    const size_t global_ids[2] = {N, M};

    internal_err = clEnqueueNDRangeKernel(
        queue, kernel, 2, NULL, global_ids, NULL, 0, NULL, profiling_evt
    );
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    *err = CL_SUCCESS;
}

void enqueue_cross_check_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   W,
    const uint32_t   H,
    cl_mem           disp_img_left,
    cl_mem           disp_img_right,
    cl_mem           out,
    cl_event        *profiling_evt,
    cl_int          *err
) {
    cl_int internal_err;
    if (err == NULL) {
        err = &internal_err;
    }

    SET_KERNEL_ARG(0, uint32_t, &N);
    SET_KERNEL_ARG(1, uint32_t, &W);
    SET_KERNEL_ARG(2, uint32_t, &H);
    SET_KERNEL_ARG(3, cl_mem, &disp_img_left);
    SET_KERNEL_ARG(4, cl_mem, &disp_img_right);
    SET_KERNEL_ARG(5, cl_mem, &out);

    const size_t global_id = N;

    internal_err = clEnqueueNDRangeKernel(
        queue, kernel, 1, NULL, &global_id, NULL, 0, NULL, profiling_evt
    );
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    *err = CL_SUCCESS;
}

void enqueue_zero_fill_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   W,
    const uint32_t   H,
    cl_mem           img,
    cl_event        *profiling_evt,
    cl_int          *err
) {
    cl_int internal_err;
    if (err == NULL) {
        err = &internal_err;
    }

    SET_KERNEL_ARG(0, uint32_t, &N);
    SET_KERNEL_ARG(1, uint32_t, &W);
    SET_KERNEL_ARG(2, uint32_t, &H);
    SET_KERNEL_ARG(3, cl_mem, &img);

    const size_t global_id = N;

    internal_err = clEnqueueNDRangeKernel(
        queue, kernel, 1, NULL, &global_id, NULL, 0, NULL, profiling_evt
    );

    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    *err = CL_SUCCESS;
}