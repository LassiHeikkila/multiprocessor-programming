#include <stdio.h>
#include <string.h>

#include "device_support.h"
#include "panic.h"

#define MAX_PANIC_MSG_LEN 32

void check_cl_error(cl_int err) {
    if (err != CL_SUCCESS) {
        char desc[MAX_PANIC_MSG_LEN];
        sprintf(desc, "CL error reported: %i", err);
        panic(desc);
    }
}

cl_program compile_program_from_file(
    const char *path, cl_context ctx, cl_int *err
) {
    cl_program program = NULL;
    FILE      *f       = NULL;
    size_t     sz      = 0;
    char      *str     = NULL;
    cl_int     internal_err;

    if (err == NULL) {
        err = &internal_err;
    }

    f = fopen(path, "rb");

    if (f == NULL) {
        panic("failed to open src file!");
    }

    // get size of program
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    rewind(f);

    str = (char *)malloc(sz + 1);
    if (str == NULL) {
        panic("failed to allocate string to hold kernel source");
    }
    str[sz] = 0;

    // read whole file into str
    fread(str, sizeof(char), sz, f);

    // create OpenCL program from str
    program = clCreateProgramWithSource(
        ctx, 1, (const char **)&str, NULL, &internal_err
    );
    free(str);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return NULL;
    }

    internal_err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return NULL;
    }

    *err = CL_SUCCESS;
    return program;
}

cl_kernel build_kernel(
    const char *kernel_name, cl_program program, cl_int *err
) {
    cl_kernel kernel = NULL;
    cl_int    internal_err;

    if (err == NULL) {
        err = &internal_err;
    }

    kernel = clCreateKernel(program, kernel_name, &internal_err);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return NULL;
    }

    *err = CL_SUCCESS;
    return kernel;
}

cl_device_id get_device(cl_int *err) {
    cl_uint        num_platforms;
    cl_int         internal_err;
    cl_platform_id platforms[MAX_NUM_CL_PLATFORMS];
    cl_device_id   device = NULL;

    if (err == NULL) {
        err = &internal_err;
    }

    internal_err =
        clGetPlatformIDs(MAX_NUM_CL_PLATFORMS, platforms, &num_platforms);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return NULL;
    }

    if (num_platforms < 1) {
        panic("no device platforms detected");
    }

    internal_err =
        clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return NULL;
    }

    *err = CL_SUCCESS;
    return device;
}

cl_context create_context(cl_device_id device, cl_int *err) {
    cl_int     internal_err;
    cl_context ctx = NULL;

    if (err == NULL) {
        err = &internal_err;
    }

    ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &internal_err);
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return NULL;
    }

    *err = CL_SUCCESS;
    return ctx;
}

cl_command_queue create_queue(
    cl_context ctx, cl_device_id device, cl_int *err
) {
    cl_int           internal_err;
    cl_command_queue queue = NULL;

    if (err == NULL) {
        err = &internal_err;
    }

    queue = clCreateCommandQueue(
        ctx, device, CL_QUEUE_PROFILING_ENABLE, &internal_err
    );
    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        return NULL;
    }

    *err = CL_SUCCESS;
    return queue;
}

uint64_t get_exec_ns(cl_event *evt) {
    // printf profiling information
    cl_ulong evt_start = 0;
    cl_ulong evt_end   = 0;

    (void)clGetEventProfilingInfo(
        *evt, CL_PROFILING_COMMAND_QUEUED, sizeof(evt_start), &evt_start, NULL
    );
    (void)clGetEventProfilingInfo(
        *evt, CL_PROFILING_COMMAND_END, sizeof(evt_end), &evt_end, NULL
    );

    return (uint64_t)(evt_end - evt_start);
}

void *read_device_memory(
    cl_command_queue queue, cl_mem mem, size_t sz, cl_int *err
) {
    uint8_t *buf = malloc(sz);
    if (buf == NULL) {
        return NULL;
    }

    cl_int internal_err;
    if (err == NULL) {
        err = &internal_err;
    }

    internal_err =
        clEnqueueReadBuffer(queue, mem, CL_TRUE, 0, sz, buf, 0, NULL, NULL);

    if (internal_err != CL_SUCCESS) {
        *err = internal_err;
        free(buf);
        return NULL;
    }

    *err = CL_SUCCESS;
    return buf;
}
