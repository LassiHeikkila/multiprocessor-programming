#ifndef _DEVICE_SUPPORT_H_
#define _DEVICE_SUPPORT_H_

#include <CL/cl.h>

#define MAX_NUM_CL_PLATFORMS 1

/*!
 * @brief Checks given error. If it is not CL_SUCCESS, calls panic.
 * @param err : error code
 */
void check_cl_error(cl_int err);

/*!
 * @brief Compiles OpenCL program from given file
 * @param path : path to source file
 * @param ctx : OpenCL context
 * @param[out] err : CL_SUCCESS for success, error code otherwise
 * @return program structure or NULL
 */
cl_program compile_program_from_file(
    const char *path, cl_context ctx, cl_int *err
);

/*!
 * @brief Creates kernel from the given program
 * @param kernel_name : A function name in the program declared with the
 * __kernel qualifier.
 * @param program : OpenCL program
 * @param[out] err : CL_SUCCESS for success, error code otherwise
 * @return kernel structure or NULL
 */
cl_kernel build_kernel(
    const char *kernel_name, cl_program program, cl_int *err
);

/*!
 * @brief Finds OpenCL device
 * @param[out] err : CL_SUCCESS for success, error code otherwise
 * @return First device id found or NULL
 */
cl_device_id get_device(cl_int *err);

/*!
 * @brief Create OpenCL context
 * @param[out] err : CL_SUCCESS for success, error code otherwise
 * @return context or NULL
 */
cl_context create_context(cl_device_id device, cl_int *err);

/*!
 * @brief Create OpenCL work queue
 * @param ctx : OpenCL context
 * @param device : OpenCL device
 * @param[out] err : CL_SUCCESS for success, error code otherwise
 * @return Work queue struct or NULL
 */
cl_command_queue create_queue(cl_context ctx, cl_device_id device, cl_int *err);

/*!
 * @brief Gets profiling data from CL runtime
 * @param[in] evt : profiling event
 * @return execution time in nanoseconds
 */
uint64_t get_exec_ns(cl_event *evt);

/*!
 * @brief Read device memory into
 * @param queue : device command queue which owns the memory
 * @param mem : device memory
 * @param sz : number of bytes to read
 * @param[out] err : CL_SUCCESS for success, error code otherwise
 * @return pointer to host buffer containing copy of device memory
 */
void *read_device_memory(
    cl_command_queue queue, cl_mem mem, size_t sz, cl_int *err
);

#endif  // _DEVICE_SUPPORT_H_