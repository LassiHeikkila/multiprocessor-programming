#ifndef _PROFILING_H_
#define _PROFILING_H_

#include <time.h>

#define PROFILING_BLOCK_DECLARE(id) \
    struct timespec profiling_##id##_begin, profiling_##id##_end

#define PROFILING_BLOCK_BEGIN(id) \
    clock_gettime(CLOCK_MONOTONIC_RAW, &profiling_##id##_begin)

#define PROFILING_BLOCK_END(id) \
    clock_gettime(CLOCK_MONOTONIC_RAW, &profiling_##id##_end)

#define PROFLING_BLOCK_CALCULATE_NS(id)                     \
    ((uint64_t)((1e9 * ((profiling_##id##_end).tv_sec -     \
                        (profiling_##id##_begin).tv_sec)) + \
                ((profiling_##id##_end).tv_nsec -           \
                 (profiling_##id##_begin).tv_nsec)))

#define PROFILING_BLOCK_PRINT_US(id)            \
    printf(                                     \
        "profiling block \"%s\" took %ld µs\n", \
        #id,                                    \
        PROFLING_BLOCK_CALCULATE_NS(id) / 1000  \
    )

#define PROFILING_BLOCK_PRINT_NS(id)            \
    printf(                                     \
        "profiling block \"%s\" took %ld µs\n", \
        #id,                                    \
        PROFLING_BLOCK_CALCULATE_NS(id)         \
    )

#endif  // _PROFILING_H_