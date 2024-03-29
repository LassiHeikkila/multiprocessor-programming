# Multiprocessor Programming
![CI status](https://github.com/LassiHeikkila/multiprocessor-programming/actions/workflows/CI.yml/badge.svg)
![Branch coverage](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fgist.githubusercontent.com%2FLassiHeikkila%2F2cc186f7491127168e70d457b16fe950%2Fraw%2Fcoverage_summary.json&query=%24.branch_percent&suffix=%25&label=Branch%20coverage&link=https%3A%2F%2Fgist.githubusercontent.com%2FLassiHeikkila%2F2cc186f7491127168e70d457b16fe950%2Fraw%2Fcoverage_summary.json)
![Line coverage](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fgist.githubusercontent.com%2FLassiHeikkila%2F2cc186f7491127168e70d457b16fe950%2Fraw%2Fcoverage_summary.json&query=%24.line_percent&suffix=%25&label=Line%20coverage&link=https%3A%2F%2Fgist.githubusercontent.com%2FLassiHeikkila%2F2cc186f7491127168e70d457b16fe950%2Fraw%2Fcoverage_summary.json)


Assignment for Multiprocessor Programming course (521288S) at University of Oulu

Group: Lassi Heikkilä

The assignment consists of several phases.
Use the links below to navigate to the phase you are interested in.

The host code is written in C.

Algorithm kernels are written using OpenCL for phases 5 and 6.

> Note: CI only tests C code in `src/`.  
> It does not build the executables from `phase_<n>`, since those require OpenCL (most of them anyway), and that requires special host support.  
> Reported coverage is for files from `src/` except for `device_support.c`  

# Phases

## Phase 1 - OpenCL setup and "Hello World"
[Phase 1](./phase_1/README.md)

## Phase 2 - OpenCL introduction
[Phase 2](./phase_2/README.md)

## Phase 3 - Stereo disparity implementation in single-threaded C code
[Phase 3](./phase_3/README.md)

## Phase 4 - Stereo disparity implementation in multi-threaded C code with OpenMP
[Phase 4](./phase_4/README.md)

## Phase 5 - Stereo disparity implementation using OpenCL for a GPU
[Phase 5](./phase_5/README.md)

## Phase 6 - Algorithmic and implementation optimization for OpenCL in CPU/GPU
[Phase 6](./phase_6/README.md)

## Phase 7 - Porting of the algorithm to WebGPU
[Phase 7](./phase_7/README.md)
