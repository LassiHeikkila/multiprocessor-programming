#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

// Defines
#define ORDER 100
#define EPSILON 1e-6f
#define MATRIX_SIZE (ORDER * ORDER * sizeof(float))

#define MADD_FLOAT_PROGRAM_FILE "madd_float_kernel.cl"
#define MMUL_FLOAT_PROGRAM_FILE "mmul_float_kernel.cl"

// Type definitions
typedef float *matrix_t;

// Declarations for functions that use OpenCL
void check_cl_error(cl_int err, const char *desc);
void init_cl_runtime(void);
void d_madd_f(matrix_t a, matrix_t b, matrix_t out, uint64_t *exec_ns);
void d_mmul_f(matrix_t a, matrix_t b, matrix_t out, uint64_t *exec_ns);

// Declarations for plain C implementations
void madd_f(matrix_t a, matrix_t b, matrix_t out, uint64_t *exec_ns);
void mmul_f(matrix_t a, matrix_t b, matrix_t out, uint64_t *exec_ns);

void rand_fill_matrix(matrix_t a);
void zero_fill_matrix(matrix_t a);
void print_matrix(matrix_t a);
void calculate_delta_matrix(matrix_t a, matrix_t b, matrix_t out);
bool matrices_equal(matrix_t a, matrix_t b, float epsilon);

// Declarations for OpenCL helpers
cl_program compile_program_from_file(
    const char *path, cl_context ctx, cl_int *err
);

// Declarations for high-level routines comparing OpenCL
// implementation to plain C version
void compare_add(matrix_t a, matrix_t b);
void compare_mul(matrix_t a, matrix_t b);

// Global data
cl_context       dev_ctx = NULL;
cl_platform_id  *dev_platforms = NULL;
cl_device_id     dev_device = NULL;
cl_command_queue dev_queue = NULL;

// Main function
int main() {
    // seed random number generator
    srand(clock());

    // uncomment below to see what clock resolution your system supports
    // mine reports 1ns resolution for monotonic clock.
    /*
    struct timespec m;
    clock_getres(CLOCK_MONOTONIC, &m);
    printf("clock resolution: %ld s %ld ns\n", m.tv_sec, m.tv_nsec);
    */

    init_cl_runtime();
    assert(dev_ctx != NULL);
    assert(dev_platforms != NULL);
    assert(dev_device != NULL);
    assert(dev_queue != NULL);

    // input matrices
    matrix_t a, b;
    a = malloc(MATRIX_SIZE);
    b = malloc(MATRIX_SIZE);
    if (a == NULL) {
        printf("failed to allocate a!\n");
        return 1;
    }
    if (b == NULL) {
        printf("failed to allocate b!\n");
        return 1;
    }

    rand_fill_matrix(a);
    rand_fill_matrix(b);

    /*
    printf("input matrices:\n");
    printf("a:\n");
    print_matrix(a);
    printf("\n");
    printf("b:\n");
    print_matrix(b);
    printf("\n");
    */

    compare_add(a, b);
    compare_mul(a, b);

    return 0;
}

// Function definitions
void rand_fill_matrix(matrix_t a) {
    // fill ORDER*ORDER matrix with random floats between -1.0 and +1.0
    for (int col = 0; col < ORDER; ++col) {
        for (int row = 0; row < ORDER; ++row) {
            a[col + (row * ORDER)] =
                (((float)rand() / (float)(RAND_MAX)) - 0.5f) * 2.0f;
        }
    }
}

void zero_fill_matrix(matrix_t a) {
    // fill ORDER*ORDER matrix with zeros
    for (int col = 0; col < ORDER; ++col) {
        for (int row = 0; row < ORDER; ++row) {
            a[col + (row * ORDER)] = 0.0f;
        }
    }
}

bool matrices_equal(matrix_t a, matrix_t b, float epsilon) {
    float delta = 0.0f;
    for (int col = 0; col < ORDER; ++col) {
        for (int row = 0; row < ORDER; ++row) {
            delta = a[col + (row * ORDER)] - b[col + (row * ORDER)];
            delta = (delta >= 0.0f ? delta : -delta);

            if (delta > epsilon) {
                printf("first delta > EPSILON at [%d][%d]\n", col, row);
                return false;
            }
        }
    }

    return true;
}

void print_matrix(matrix_t a) {
    // print ORDER*ORDER matrix
    for (int col = 0; col < ORDER; ++col) {
        for (int row = 0; row < ORDER; ++row) {
            printf("%0.3f\t", a[col + (row * ORDER)]);
        }
        printf("\n");
    }
}

void calculate_delta_matrix(matrix_t a, matrix_t b, matrix_t out) {
    float delta = 0.0f;
    for (int col = 0; col < ORDER; ++col) {
        for (int row = 0; row < ORDER; ++row) {
            delta = a[col + (row * ORDER)] - b[col + (row * ORDER)];
            out[col + (row * ORDER)] = (delta < 0 ? -delta : delta);
        }
    }
}

void check_cl_error(cl_int err, const char *desc) {
    if (err != CL_SUCCESS) {
        printf("Error in step: %s\n", desc);
        exit(err);
    }
}

void init_cl_runtime(void) {
    cl_int  err;
    cl_uint num_platforms;

    // Determine platform
    int max_platforms = 1;
    err = clGetPlatformIDs(max_platforms, NULL, &num_platforms);
    check_cl_error(err, "getting platform ids");
    printf("Number of platforms detected: %d\n", num_platforms);

    dev_platforms =
        (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
    err = clGetPlatformIDs(max_platforms, dev_platforms, &num_platforms);
    check_cl_error(err, "getting platform ids");

    if (num_platforms < 1) {
        printf("No platform detected, exit!\n");
        exit(1);
    }

    // Get device
    err = clGetDeviceIDs(
        dev_platforms[0], CL_DEVICE_TYPE_ALL, 1, &dev_device, NULL
    );
    check_cl_error(err, "getting device ids");

    // Create context
    dev_ctx = clCreateContext(NULL, 1, &dev_device, NULL, NULL, &err);
    check_cl_error(err, "getting context");

    // Create queue
    dev_queue = clCreateCommandQueue(
        dev_ctx, dev_device, CL_QUEUE_PROFILING_ENABLE, &err
    );
    check_cl_error(err, "creating command queue");
}

cl_program compile_program_from_file(
    const char *path, cl_context ctx, cl_int *err
) {
    cl_program program = NULL;
    FILE      *f = NULL;
    size_t     sz = 0;
    char      *str = NULL;

    f = fopen(path, "rb");

    if (f == NULL) {
        return NULL;
    }

    // get size of program
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    rewind(f);

    str = (char *)malloc(sz + 1);
    if (str == NULL) {
        // failed to allocate
        return NULL;
    }
    str[sz] = 0;

    // read whole file into str
    fread(str, sizeof(char), sz, f);

    // create OpenCL program from str
    program = clCreateProgramWithSource(ctx, 1, (const char **)&str, NULL, err);
    if (err && *err != CL_SUCCESS) {
        free(str);
        printf("failed to create program: %i\n", *err);
        return program;
    }

    cl_int build_res = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (build_res != CL_SUCCESS && err != NULL) {
        *err = build_res;
    }

    free(str);

    return program;
}

void madd_f(matrix_t a, matrix_t b, matrix_t out, uint64_t *exec_ns) {
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

    for (int col = 0; col < ORDER; ++col) {
        for (int row = 0; row < ORDER; ++row) {
            out[col + (row * ORDER)] =
                a[col + (row * ORDER)] + b[col + (row * ORDER)];
        }
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    if (exec_ns != NULL) {
        *exec_ns = (uint64_t)((1e9 * (end.tv_sec - begin.tv_sec)) +
                              (end.tv_nsec - begin.tv_nsec));
    }
}

void mmul_f(matrix_t a, matrix_t b, matrix_t out, uint64_t *exec_ns) {
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

    double tmp = 0.0;
    for (int col = 0; col < ORDER; ++col) {
        for (int row = 0; row < ORDER; ++row) {
            tmp = 0.0;
            for (int k = 0; k < ORDER; ++k) {
                tmp += a[k + (row * ORDER)] * b[col + (k * ORDER)];
            }
            out[col + (row * ORDER)] = (float)tmp;
        }
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    if (exec_ns != NULL) {
        *exec_ns = (uint64_t)((1e9 * (end.tv_sec - begin.tv_sec)) +
                              (end.tv_nsec - begin.tv_nsec));
    }
}

void d_madd_f(matrix_t a, matrix_t b, matrix_t out, uint64_t *exec_ns) {
    cl_int     err;
    cl_program program;
    cl_kernel  kernel;
    cl_event   profiling_evt;
    // device buffers for input matrices and output matrix
    cl_mem     d_a, d_b, d_out;

    // compile the kernel, set up the execution environment
    // this will not be considered when measuring performance

    program = compile_program_from_file(MADD_FLOAT_PROGRAM_FILE, dev_ctx, &err);
    check_cl_error(err, "compiling madd program");

    kernel = clCreateKernel(program, "madd", &err);
    check_cl_error(err, "creating madd kernel");

    // create device memory buffers
    d_a = clCreateBuffer(
        dev_ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, MATRIX_SIZE, a, &err
    );
    check_cl_error(err, "creating device input buffer a");

    d_b = clCreateBuffer(
        dev_ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, MATRIX_SIZE, b, &err
    );
    check_cl_error(err, "creating device input buffer a");

    d_out = clCreateBuffer(
        dev_ctx,
        CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
        MATRIX_SIZE,
        out,
        &err
    );
    check_cl_error(err, "creating device output buffer");

    // set kernel arguments
    int N = ORDER;
    err = clSetKernelArg(kernel, 0, sizeof(int), &N);
    check_cl_error(err, "setting kernel argument 0");
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_a);
    check_cl_error(err, "setting kernel argument 1");
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_b);
    check_cl_error(err, "setting kernel argument 2");
    err = clSetKernelArg(kernel, 3, sizeof(cl_mem), &d_out);
    check_cl_error(err, "setting kernel argument 3");

    // enqueue the work items and wait for them to finish
    // Execute the kernel N*N times
    const size_t global_ids[2] = {ORDER, ORDER};
    err = clEnqueueNDRangeKernel(
        dev_queue, kernel, 2, NULL, global_ids, NULL, 0, NULL, &profiling_evt
    );

    check_cl_error(err, "enqueuing task");

    err = clFinish(dev_queue);
    check_cl_error(err, "waiting for jobs to finish");

    // read output
    // get profiling information
    (void)clWaitForEvents(1, &profiling_evt);

    // Read kernel output
    err = clEnqueueReadBuffer(
        dev_queue, d_out, CL_TRUE, 0, MATRIX_SIZE, out, 0, NULL, NULL
    );
    check_cl_error(err, "reading output");

    // printf profiling information
    cl_ulong evt_start = 0;
    cl_ulong evt_end = 0;
    size_t   return_bytes = 0;

    (void)clGetEventProfilingInfo(
        profiling_evt,
        CL_PROFILING_COMMAND_QUEUED,
        sizeof(evt_start),
        &evt_start,
        &return_bytes
    );
    (void)clGetEventProfilingInfo(
        profiling_evt,
        CL_PROFILING_COMMAND_END,
        sizeof(evt_end),
        &evt_end,
        &return_bytes
    );

    if (exec_ns != NULL) {
        *exec_ns = (uint64_t)(evt_end - evt_start);
    }
}

void d_mmul_f(matrix_t a, matrix_t b, matrix_t out, uint64_t *exec_ns) {
    cl_int     err;
    cl_program program;
    cl_kernel  kernel;
    cl_event   profiling_evt;
    // device buffers for input matrices and output matrix
    cl_mem     d_a, d_b, d_out;

    // compile the kernel, set up the execution environment
    // this will not be considered when measuring performance

    program = compile_program_from_file(MMUL_FLOAT_PROGRAM_FILE, dev_ctx, &err);
    check_cl_error(err, "compiling mmul program");

    kernel = clCreateKernel(program, "mmul", &err);
    check_cl_error(err, "creating mmul kernel");

    // create device memory buffers
    d_a = clCreateBuffer(
        dev_ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, MATRIX_SIZE, a, &err
    );
    check_cl_error(err, "creating device input buffer a");

    d_b = clCreateBuffer(
        dev_ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, MATRIX_SIZE, b, &err
    );
    check_cl_error(err, "creating device input buffer a");

    d_out = clCreateBuffer(
        dev_ctx,
        CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
        MATRIX_SIZE,
        out,
        &err
    );
    check_cl_error(err, "creating device output buffer");

    // set kernel arguments
    int N = ORDER;
    err = clSetKernelArg(kernel, 0, sizeof(int), &N);
    check_cl_error(err, "setting kernel argument 0");
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_a);
    check_cl_error(err, "setting kernel argument 1");
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_b);
    check_cl_error(err, "setting kernel argument 2");
    err = clSetKernelArg(kernel, 3, sizeof(cl_mem), &d_out);
    check_cl_error(err, "setting kernel argument 3");

    // enqueue the work items and wait for them to finish
    // Execute the kernel N*N times
    const size_t global_ids[2] = {ORDER, ORDER};
    err = clEnqueueNDRangeKernel(
        dev_queue, kernel, 2, NULL, global_ids, NULL, 0, NULL, &profiling_evt
    );

    check_cl_error(err, "enqueuing task");

    err = clFinish(dev_queue);
    check_cl_error(err, "waiting for jobs to finish");

    // read output
    // get profiling information
    (void)clWaitForEvents(1, &profiling_evt);

    // Read kernel output
    err = clEnqueueReadBuffer(
        dev_queue, d_out, CL_TRUE, 0, MATRIX_SIZE, out, 0, NULL, NULL
    );
    check_cl_error(err, "reading output");

    // printf profiling information
    cl_ulong evt_start = 0;
    cl_ulong evt_end = 0;
    size_t   return_bytes = 0;

    (void)clGetEventProfilingInfo(
        profiling_evt,
        CL_PROFILING_COMMAND_QUEUED,
        sizeof(evt_start),
        &evt_start,
        &return_bytes
    );
    (void)clGetEventProfilingInfo(
        profiling_evt,
        CL_PROFILING_COMMAND_END,
        sizeof(evt_end),
        &evt_end,
        &return_bytes
    );

    if (exec_ns != NULL) {
        *exec_ns = (uint64_t)(evt_end - evt_start);
    }
}

void compare_add(matrix_t a, matrix_t b) {
    printf("comparing addition:\n");

    // output matrices
    matrix_t out_plain, out_cl;
    out_plain = malloc(MATRIX_SIZE);
    if (out_plain == NULL) {
        printf("failed to allocate out_plain!\n");
        goto compare_add_end;
    }
    out_cl = malloc(MATRIX_SIZE);
    if (out_cl == NULL) {
        printf("failed to allocate out_cl!\n");
        goto compare_add_end;
    }

    // for profiling
    uint64_t dur_plain, dur_cl;

    zero_fill_matrix(out_plain);
    zero_fill_matrix(out_cl);

    // plain
    madd_f(a, b, out_plain, &dur_plain);

    // opencl
    // note that this only reports the kernel execution time
    // and there is considerable overhead for setting up the kernel and copying
    // data back and forth between host and device
    d_madd_f(a, b, out_cl, &dur_cl);

    if (!matrices_equal(out_plain, out_cl, EPSILON)) {
        printf("output matrices not equal!\n");
        matrix_t diff = malloc(MATRIX_SIZE);
        // TODO: handle failed allocation
        calculate_delta_matrix(out_plain, out_cl, diff);
        print_matrix(diff);
    }

    printf("plain:  %ld ns\n", dur_plain);
    printf("opencl: %ld ns\n", dur_cl);
    printf("\n");

compare_add_end:
    free(out_plain);
    free(out_cl);
}

void compare_mul(matrix_t a, matrix_t b) {
    printf("comparing multiplication:\n");

    // output matrices
    matrix_t out_plain, out_cl;

    out_plain = malloc(MATRIX_SIZE);
    if (out_plain == NULL) {
        printf("failed to allocate out_plain!\n");
        goto compare_mul_end;
    }

    out_cl = malloc(MATRIX_SIZE);
    if (out_cl == NULL) {
        printf("failed to allocate out_cl!\n");
        goto compare_mul_end;
    }

    // for profiling
    uint64_t dur_plain, dur_cl;

    zero_fill_matrix(out_plain);
    zero_fill_matrix(out_cl);

    // plain
    mmul_f(a, b, out_plain, &dur_plain);

    // opencl
    // note that this only reports the kernel execution time
    // and there is considerable overhead for setting up the kernel and copying
    // data back and forth between host and device
    d_mmul_f(a, b, out_cl, &dur_cl);

    if (!matrices_equal(out_plain, out_cl, EPSILON)) {
        printf("output matrices not equal!\n");
        matrix_t diff = malloc(MATRIX_SIZE);
        // TODO: handle failed allocation
        calculate_delta_matrix(out_plain, out_cl, diff);
        print_matrix(diff);
    }

    printf("plain:  %ld ns\n", dur_plain);
    printf("opencl: %ld ns\n", dur_cl);
    printf("\n");

compare_mul_end:
    free(out_plain);
    free(out_cl);
}