#include <stdio.h>   // printf
#include <stdlib.h>  // malloc, etc.
#include <string.h>  // memset

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

const char* kernel_source =
    "__kernel void hello(__global char *output) {"
    "output[0] = 'h';"
    "output[1] = 'e';"
    "output[2] = 'l';"
    "output[3] = 'l';"
    "output[4] = 'o';"
    "output[5] = ' ';"
    "output[6] = 'w';"
    "output[7] = 'o';"
    "output[8] = 'r';"
    "output[9] = 'l';"
    "output[10] = 'd';"
    "output[11] = '!';"
    "output[12] = '\\n';"
    "}";

int main() {
    cl_int           err;
    cl_uint          num_platforms;
    cl_platform_id*  platforms;
    cl_device_id     device;
    cl_context       context;
    cl_command_queue queue;
    cl_program       program;
    cl_kernel        kernel;
    cl_mem           output;

    // result needs to be one bigger than the output since the output doesn't
    // contain a NULL terminator
    char result[14];
    // zero-fill the result so it ends with NULL
    // (assuming the output is shorter than the buffer...)
    (void)memset(result, 0, sizeof(result));

    // Determine platform
    int max_platforms = 1;
    err = clGetPlatformIDs(max_platforms, NULL, &num_platforms);
    // TODO: handle error
    printf("Number of platforms detected: %d\n", num_platforms);

    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
    err = clGetPlatformIDs(max_platforms, platforms, &num_platforms);
    // TODO: handle error

    if (num_platforms < 1) {
        printf("No platform detected, exit!\n");
        exit(1);
    }

    // Get device
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 1, &device, NULL);
    // TODO: handle error

    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    // TODO: handle error

    // Create queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    // TODO: handle error

    // Read kernel code and compile it
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    // TODO: handle errors

    // Create kernel and kernel parameters
    kernel = clCreateKernel(program, "hello", &err);
    output =
        clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(result), NULL, &err);
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output);
    // TODO: handle errors

    // Execute the kernel
    err = clEnqueueTask(queue, kernel, 0, NULL, NULL);
    // TODO: handle error

    // Read kernel output
    err = clEnqueueReadBuffer(queue, output, CL_TRUE, 0, sizeof(result), result,
                              0, NULL, NULL);
    // TODO: handle error

    printf("%s", result);

    // Free allocated objects and memory
    clReleaseMemObject(output);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(platforms);

    return 0;
}