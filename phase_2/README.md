# Phase 2 - OpenCL introduction

## Summary of goals
A simple set of routines working on OpenCL, with auxiliary functions.

## Matrix multiplication with OpenCL
[main.c](main.c) contains a C program which compiles the OpenCL kernel from [mmul_kernel.cl](mmul_kernel.cl) to do matrix multiplication of two 3x3 matrices.

The program was heavily inspired by the [Hands On OpenCL Exercise 6](https://github.com/HandsOnOpenCL/Exercises-Solutions/blob/master/Solutions/Exercise06/README.md).

The program only calculates the following, the input matrices are not configurable at the moment:
```math

\begin{pmatrix}
1 & 2 & 3 \\
4 & 5 & 6 \\
7 & 8 & 9 \\
\end{pmatrix}

\begin{pmatrix}
1 & 2 & 3 \\
4 & 5 & 6 \\
7 & 8 & 9 \\
\end{pmatrix}

=

\begin{pmatrix}
30  &  36 & 42 \\
66  &  81 & 96 \\
102 & 126 & 150 \\
\end{pmatrix}

```

## Output
```console
$ ./main
Number of platforms detected: 1
result matrix:
30      36      42
66      81      96
102     126     150
result matrix is correct!
```

[< Back to top](../README.md)
