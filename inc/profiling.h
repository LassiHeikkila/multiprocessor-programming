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

#define PROFILING_BLOCK_PRINT_S(id)                              \
    printf(                                                      \
        "profiling block \"%s\" took %0.3f s\n",                 \
        #id,                                                     \
        (double)(PROFLING_BLOCK_CALCULATE_NS(id)) / 1000000000.0 \
    )

#define PROFILING_BLOCK_PRINT_MS(id)                          \
    printf(                                                   \
        "profiling block \"%s\" took %0.3f ms\n",             \
        #id,                                                  \
        (double)(PROFLING_BLOCK_CALCULATE_NS(id)) / 1000000.0 \
    )

#define PROFILING_BLOCK_PRINT_US(id)            \
    printf(                                     \
        "profiling block \"%s\" took %ld µs\n", \
        #id,                                    \
        PROFLING_BLOCK_CALCULATE_NS(id) / 1000  \
    )

#define PROFILING_BLOCK_PRINT_NS(id)            \
    printf(                                     \
        "profiling block \"%s\" took %ld ns\n", \
        #id,                                    \
        PROFLING_BLOCK_CALCULATE_NS(id)         \
    )

#define PROFILING_RAW_PRINT_S(desc, ns) \
    printf("\"%s\" took %0.3f s\n", desc, ((double)ns / 1000000000.0));

#define PROFILING_RAW_PRINT_MS(desc, ns) \
    printf("\"%s\" took %0.3f ms\n", desc, ((double)ns / 1000000.0));

#define PROFILING_RAW_PRINT_US(desc, ns) \
    printf("\"%s\" took %ld µs\n", desc, (ns / 1000));

#define PROFILING_RAW_PRINT_NS(desc, ns) \
    printf("\"%s\" took %ld ns\n", desc, (ns));

#endif  // _PROFILING_H_