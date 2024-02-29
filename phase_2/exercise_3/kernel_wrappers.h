#ifndef _KERNEL_WRAPPERS_H_
#define _KERNEL_WRAPPERS_H_

#include <stdint.h>

#include <CL/cl.h>

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
    const uint32_t   Wi,
    const uint32_t   Hi,
    cl_mem           img_in,
    cl_mem           img_out,
    cl_event        *profiling_evt,
    cl_int          *err
);

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
);

#endif  // _KERNEL_WRAPPERS_H_