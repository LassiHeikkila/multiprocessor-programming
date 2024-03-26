#include <stdio.h>
#include <string.h>

#include "device_support.h"
#include "panic.h"

#define MAX_PANIC_MSG_LEN 32
#define MAX_PANIC_MSG_WITH_FILE_LINE_LEN 256

void check_cl_error(cl_int err) {
    if (err != CL_SUCCESS) {
        char desc[MAX_PANIC_MSG_LEN];
        sprintf(desc, "CL error reported: %i", err);
        panic(desc);
    }
}

void check_cl_error_with_file_line(const char *file, int line, cl_int err) {
    if (err != CL_SUCCESS) {
        char desc[MAX_PANIC_MSG_WITH_FILE_LINE_LEN];
        sprintf(desc, "CL error reported: %i on %s:%d", err, file, line);
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

uint64_t get_exec_ns(cl_event evt) {
    if (evt == NULL) {
        return 0;
    }

    // printf profiling information
    cl_ulong evt_start = 0;
    cl_ulong evt_end   = 0;

    (void)clGetEventProfilingInfo(
        evt, CL_PROFILING_COMMAND_QUEUED, sizeof(evt_start), &evt_start, NULL
    );
    (void)clGetEventProfilingInfo(
        evt, CL_PROFILING_COMMAND_END, sizeof(evt_end), &evt_end, NULL
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

#define GET_DEVICE_INFO(d, info, t)                            \
    do {                                                       \
        t param = 0;                                           \
        clGetDeviceInfo((d), (info), sizeof(t), &param, NULL); \
        printf(#info ": %zu\n", (uint64_t)param);              \
    } while (0);

void print_device_info(cl_device_id dev) {
    printf("Device info dump:\n\n");
    {
        cl_device_type t;
        clGetDeviceInfo(dev, CL_DEVICE_TYPE, sizeof(cl_device_type), &t, NULL);
        switch (t) {
            case CL_DEVICE_TYPE_CPU:
                printf("Device is a CPU\n");
                break;
            case CL_DEVICE_TYPE_GPU:
                printf("Device is a GPU\n");
                break;
            case CL_DEVICE_TYPE_ACCELERATOR:
                printf("Device is an ACCELERATOR\n");
                break;
            default:
                printf("Device is of type 0x%lx\n", t);
                break;
        }
    }
    {
        char device_vendor_buf[64];
        char device_name_buf[64];
        memset(&device_vendor_buf, 0, sizeof(device_vendor_buf));
        memset(&device_name_buf, 0, sizeof(device_name_buf));

        clGetDeviceInfo(
            dev,
            CL_DEVICE_VENDOR,
            sizeof(device_vendor_buf) - 1,
            &device_vendor_buf[0],
            NULL
        );
        clGetDeviceInfo(
            dev,
            CL_DEVICE_NAME,
            sizeof(device_name_buf) - 1,
            &device_name_buf[0],
            NULL
        );
        printf(
            "Device vendor: %s\nDevice name: %s\n",
            device_vendor_buf,
            device_name_buf
        );
    }
    {
        cl_device_local_mem_type t;
        clGetDeviceInfo(
            dev,
            CL_DEVICE_LOCAL_MEM_TYPE,
            sizeof(cl_device_local_mem_type),
            &t,
            NULL
        );
        switch (t) {
            case CL_LOCAL:
                printf("Device supports dedicated local memory\n");
                break;
            case CL_GLOBAL:
                printf("Device supports only global device memory\n");
                break;
            case CL_NONE:
                printf("Device does not support any kind of local memory\n");
                break;
        }
    }
    {
        cl_ulong s = 0;
        clGetDeviceInfo(
            dev, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &s, NULL
        );
        printf("Device local memory max size is %zu bytes\n", s);
    }
    {
        cl_uint cu = 0;
        size_t  w  = 0;
        clGetDeviceInfo(
            dev, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &cu, NULL
        );
        clGetDeviceInfo(
            dev, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &w, NULL
        );
        printf(
            "Device supports up to %u compute units with up to %zu work groups "
            "each\n",
            cu,
            w
        );
    }
    {
        cl_uint f = 0;
        clGetDeviceInfo(
            dev, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &f, NULL
        );
        printf("Device has maximum clock frequency of %u MHz\n", f);
    }
    {
        cl_ulong b = 0;
        clGetDeviceInfo(
            dev, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(cl_ulong), &b, NULL
        );
        printf("Device supports constant buffers up to %zu bytes\n", b);
    }
    {
        cl_uint dimensions = 0;
        // hard coded to up to eight, don't want to dynamically allocate here...
        // My host machine reports up to three dimensions so 8 should be
        // plenty.
        size_t  sizes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        clGetDeviceInfo(
            dev,
            CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
            sizeof(cl_uint),
            &dimensions,
            NULL
        );
        clGetDeviceInfo(
            dev,
            CL_DEVICE_MAX_WORK_ITEM_SIZES,
            sizeof(size_t) * dimensions,
            &sizes[0],
            NULL
        );
        printf("Device supports up to %u work item dimensions\n", dimensions);
        for (uint32_t d = 0; d < dimensions; ++d) {
            printf(
                "Device work item dimension %u supports up to %zu work items\n",
                d,
                sizes[d]
            );
        }
    }
    {
        cl_bool images_supported = CL_FALSE;
        clGetDeviceInfo(
            dev,
            CL_DEVICE_IMAGE_SUPPORT,
            sizeof(cl_bool),
            &images_supported,
            NULL
        );
        if (images_supported) {
            printf("Device supports images\n");
        } else {
            printf("Device does not support images\n");
        }
    }
    printf("\nend of device info dump\n\n");
}
