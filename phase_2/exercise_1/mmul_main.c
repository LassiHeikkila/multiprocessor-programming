#include <stdbool.h>
#include <stdio.h>   // printf
#include <stdlib.h>  // malloc, etc.
#include <string.h>  // memset

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#define ORDER 3
const int CORRECT_ANSWER[ORDER][ORDER] = {
    {30, 36, 42}, {66, 81, 96}, {102, 126, 150}};

void       check_error(cl_int err, const char *desc);

cl_program compile_program_from_file(const char *path, cl_context ctx,
                                     cl_int *err);

bool check_matrices_equal(const int A[ORDER][ORDER], const int B[ORDER][ORDER]);

int  main() {
    cl_int           err;
    cl_uint          num_platforms;
    cl_platform_id  *platforms;
    cl_device_id     device;
    cl_context       context;
    cl_command_queue queue;
    cl_program       program;
    cl_kernel        kernel;
    cl_event         profiling_evt;
    // result needs to be one bigger than the output since the output doesn't
    // contain a NULL terminator
    int N = ORDER;
    int A[ORDER][ORDER] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int B[ORDER][ORDER] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int C[ORDER][ORDER] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    // Determine platform
    int max_platforms = 1;
    err = clGetPlatformIDs(max_platforms, NULL, &num_platforms);
    check_error(err, "getting platform ids");
    printf("Number of platforms detected: %d\n", num_platforms);

    platforms =
        (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
    err = clGetPlatformIDs(max_platforms, platforms, &num_platforms);
    check_error(err, "getting platform ids");

    if (num_platforms < 1) {
        printf("No platform detected, exit!\n");
        exit(1);
    }

    // Get device
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 1, &device, NULL);
    check_error(err, "getting device ids");

    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    check_error(err, "getting context");

    // Create queue
    queue =
        clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    check_error(err, "creating command queue");

    program = compile_program_from_file("mmul_int_kernel.cl", context, &err);
    check_error(err, "compiling kernel program");
    if (program == NULL) {
        printf("got NULL program\n");
        exit(1);
    }

    // Create kernel and kernel parameters
    kernel = clCreateKernel(program, "mmul", &err);
    check_error(err, "creating kernel");

    // create device buffers for input matrices and output matrix
    cl_mem d_A, d_B, d_C;

    d_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          sizeof(A), A, &err);
    check_error(err, "creating device buffer A");

    d_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          sizeof(B), B, &err);
    check_error(err, "creating device buffer B");

    d_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
                          sizeof(C), C, &err);
    check_error(err, "creating device buffer C");

    err = clSetKernelArg(kernel, 0, sizeof(int), &N);
    check_error(err, "setting kernel argument 0");
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_A);
    check_error(err, "setting kernel argument 1");
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_B);
    check_error(err, "setting kernel argument 2");
    err = clSetKernelArg(kernel, 3, sizeof(cl_mem), &d_C);
    check_error(err, "setting kernel argument 3");

    // Execute the kernel N*N times
    const size_t global_ids[2] = {ORDER, ORDER};
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_ids, NULL, 0,
                                  NULL, &profiling_evt);

    check_error(err, "enqueueueueing task");

    err = clFinish(queue);
    check_error(err, "waiting for jobs to finish");

    // get profiling information
    (void)clWaitForEvents(1, &profiling_evt);

    // Read kernel output
    err = clEnqueueReadBuffer(queue, d_C, CL_TRUE, 0, sizeof(C), C, 0, NULL,
                               NULL);
    check_error(err, "reading output");

    printf("result matrix:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%d\t", C[i][j]);
        }
        printf("\n");
    }

    if (!check_matrices_equal(C, CORRECT_ANSWER)) {
        printf("result matrix is incorrect!\n");

        printf("expected:\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                printf("%d\t", CORRECT_ANSWER[i][j]);
            }
            printf("\n");
        }
    } else {
        printf("result matrix is correct!\n");
    }

    // printf profiling information
    cl_ulong evt_start = 0;
    cl_ulong evt_end = 0;
    size_t   return_bytes = 0;

    (void)clGetEventProfilingInfo(profiling_evt, CL_PROFILING_COMMAND_QUEUED,
                                   sizeof(evt_start), &evt_start, &return_bytes);
    (void)clGetEventProfilingInfo(profiling_evt, CL_PROFILING_COMMAND_END,
                                   sizeof(evt_end), &evt_end, &return_bytes);
    double runtime = (double)(evt_end - evt_start);

    printf("\nRuntime of the kernel was %.4f microseconds\n", runtime * 1e-3);

    // Free allocated objects and memory
    clReleaseMemObject(d_A);
    clReleaseMemObject(d_B);
    clReleaseMemObject(d_C);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(platforms);

    return 0;
}

cl_program compile_program_from_file(const char *path, cl_context ctx,
                                     cl_int *err) {
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

void check_error(cl_int err, const char *desc) {
    if (err != CL_SUCCESS) {
        printf("Error in step: %s\n", desc);
        exit(err);
    }
}

bool check_matrices_equal(const int A[ORDER][ORDER],
                          const int B[ORDER][ORDER]) {
    for (int c = 0; c < ORDER; ++c) {
        for (int r = 0; r < ORDER; ++r) {
            if (A[r][c] != B[r][c]) {
                return false;
            }
        }
    }
    return true;
}