# Phase 2 - OpenCL introduction

## Summary of goals
A simple set of routines working on OpenCL, with auxiliary functions.

Program(s) should be able to do:
- matrix addition
- grayscale conversion
- filtering

It should also be able to measure time taken by a subroutine.

It should also be able to use OpenCL profiling utilities.

## Matrix addition (and multiplication) with OpenCL
[exercise_1/main.c](./exercise_1/main.c) contains a C program which compiles the OpenCL kernels from [madd_float_kernel.cl](./exercise_1/madd_float_kernel.cl) and [mmul_float_kernel.cl](./exercise_1/mmul_float_kernel.cl), comparing their performance to plain C implementations of matrix addition and multiplication.

The program was heavily inspired by the [Hands On OpenCL Exercise 6](https://github.com/HandsOnOpenCL/Exercises-Solutions/blob/master/Solutions/Exercise06/README.md).

The [exercise_1 directory](./exercise_1/) also contains [mmul_main.c](./exercise_1/mmul_main.c) and [madd_main.c](./exercise_1/madd_main.c) as well as corresponding [mmul_int_kernel.cl](./exercise_1/mmul_int_kernel.cl) and [madd_int_kernel.cl](./exercise_1/madd_int_kernel.cl) which do matrix multiplication and addition with integers for a fixed 3x3, verifying the output is correct.

### Output
Using 100x100 matrices in the following case:
```console
$ ./main
Number of platforms detected: 1
comparing addition:
plain:  22623 ns
opencl: 26656 ns

comparing multiplication:
plain:  2310410 ns
opencl: 201632 ns

```

## Grayscale conversion, image resizing and filtering


[< Back to top](../README.md)
