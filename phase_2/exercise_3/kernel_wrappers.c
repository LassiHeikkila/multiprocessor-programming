#include <stdint.h>

#include <CL/cl.h>

#include "kernel_wrappers.h"

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

    internal_err = clSetKernelArg(kernel, 0, sizeof(uint32_t), &N);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 1, sizeof(uint32_t), &Wi);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 2, sizeof(uint32_t), &Hi);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 3, sizeof(uint32_t), &Wo);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 4, sizeof(uint32_t), &Ho);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 5, sizeof(cl_mem), &img_in);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 6, sizeof(cl_mem), &img_out);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    const size_t global_ids[2] = {N, N};

    internal_err = clEnqueueNDRangeKernel(
        queue, kernel, 2, NULL, global_ids, NULL, 0, NULL, profiling_evt
    );
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    *err = CL_SUCCESS;
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

    internal_err = clSetKernelArg(kernel, 0, sizeof(uint32_t), &N);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 1, sizeof(uint32_t), &Wi);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 2, sizeof(uint32_t), &Hi);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 3, sizeof(cl_mem), &img_in);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 4, sizeof(cl_mem), &img_out);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    const size_t global_ids[2] = {N, N};

    internal_err = clEnqueueNDRangeKernel(
        queue, kernel, 2, NULL, global_ids, NULL, 0, NULL, profiling_evt
    );
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    *err = CL_SUCCESS;
}

void enqueue_filtering_work(
    cl_command_queue queue,
    cl_kernel        kernel,
    const uint32_t   N,
    const uint32_t   Wi,
    const uint32_t   Hi,
    const int32_t    R,
    cl_mem           img_in,
    cl_mem           img_out,
    cl_event        *profiling_evt,
    cl_int          *err
) {
    cl_int internal_err;
    if (err == NULL) {
        err = &internal_err;
    }

    internal_err = clSetKernelArg(kernel, 0, sizeof(uint32_t), &N);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 1, sizeof(uint32_t), &Wi);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 2, sizeof(uint32_t), &Hi);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 3, sizeof(int32_t), &R);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 4, sizeof(cl_mem), &img_in);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    internal_err = clSetKernelArg(kernel, 5, sizeof(cl_mem), &img_out);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    const size_t global_ids[2] = {N, N};

    internal_err = clEnqueueNDRangeKernel(
        queue, kernel, 2, NULL, global_ids, NULL, 0, NULL, profiling_evt
    );
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return;
    }

    *err = CL_SUCCESS;
}