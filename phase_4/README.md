# Phase 4 - Stereo disparity implementation in multi-threaded C code with OpenMP

## Summary of goals
A C implementation that utilize more than one core of the CPU.

## Implementation
Implementation of the program is almost the same as in [phase_3](../phase_3/README.md).

Only difference is that some of the top-level for-loops have been parallelized with `#pragma omp parallel for`

Parallelized loops:
- extracting data windows
- calculating ZNCC data
  - left to right
  - right to left
- postprocessing
  - cross-checking
  - zero-value filling

Almost all loops could be parallelized simply by adding the `#pragma omp parallel for` statement.

The zero-value filling required some more through than the others due to how it uses pre-allocated memory for the BFS visited map and FIFO.
Here it was necessary to use the `#pragma omp parallel` statement, and manually use the thread number and total number of threads when accessing image rows.
Each thread allocated it's own data for BFS.

### Output

Example output from parallelized version:

```console
$ make run
Running phase 4
bin/main
loading images...
pre-processing images...
pre-processing data windows...
computing depthmap left to right:
computing depthmap right to left:
outputting raw depthmaps
cross-checking...
filling empty regions...
output crosschecked depthmap
profiling block "preprocessing" took 87.604 ms
profiling block "zncc_calculation" took 0.417 s
profiling block "postprocessing" took 50.990 ms
```

Reference output from single-threaded version:
```console
$ make run    
Running phase 3
bin/main
loading images...
pre-processing images...
pre-processing data windows...
computing depthmap left to right:
computing depthmap right to left:
outputting raw depthmaps
cross-checking...
filling empty regions...
output crosschecked depthmap
profiling block "preprocessing" took 439.894 ms
profiling block "zncc_calculation" took 2.833 s
profiling block "postprocessing" took 269.609 ms
```

> NOTE: both versions have progress printing disabled as it may impact execution time.

| Phase              | Execution time (single threaded) | Execution time (parallelized) | Speed up (ratio) |
| ------------------ | -------------------------------: | ----------------------------: | ---------------: |
| `preprocessing`    |                       439.894 ms |                     87.604 ms |             5.02 |
| `zncc_calculation` |                          2.833 s |                       0.417 s |             6.79 |
| `postprocessing`   |                       269.609 ms |                     50.990 ms |             5.29 |

The system used for these tests has 12 logical threads running on 6 physical CPU cores.
OpenMP `omp_get_max_threads()` reports 12.

We can see that each phase is accelerated by 5-6x.

The generated depthmap is found [here](./output_images/depthmap_cc.png) together with intermediate images.
The result is identical to the single-threaded version.

[< Back to top](../README.md)
