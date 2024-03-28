#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "munit.h"

#include "coord_fifo.h"
#include "image_operations.h"
#include "zncc_operations.h"

MunitResult test_calculate_window_mean(
    const MunitParameter params[], void* data
) {
    (void)params;
    (void)data;

    typedef struct test_case {
        double   data[16];  // up to 16, can be less
        uint32_t W;
        uint32_t H;
        double   expect;
    } test_case_t;

    // clang-format off
    test_case_t test_cases[] = {
        { .data = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 }, .W = 10, .H = 1, .expect = 5.5 },
        { .data = { 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0 },  .W = 10, .H = 1, .expect = 2.0 },
        { .data = {
            2.0, 2.0, 2.0, 2.0, 2.0, 
            2.0, 2.0, 2.0, 2.0, 2.0
         },  .W = 5, .H = 2, .expect = 2.0 },
        { .data = {
             1.0,  1.0,  1.0,  1.0, 
            -1.0, -1.0, -1.0, -1.0, 
             0.0,  0.0,  0.0,  0.0, 
            -1.0,  1.0, -1.0,  1.0
         }, .W = 4, .H = 4, .expect = 0.0 },
        { .data = { 100.0, 200.0, -200.0, 400.0, -300.0 }, .W = 5, .H =1, .expect = 40.0}
    };
    // clang-format on

    for (uint32_t i = 0; i < (sizeof(test_cases) / sizeof(test_case_t)); ++i) {
        double got = calculate_window_mean(
            test_cases[i].data, test_cases[i].W, test_cases[i].H
        );

        munit_assert_double_equal(test_cases[i].expect, got, 4);
    }

    return MUNIT_OK;
}

MunitResult test_calculate_window_standard_deviation(
    const MunitParameter params[], void* data
) {
    (void)params;
    (void)data;

    typedef struct test_case {
        double   data[16];  // up to 16, can be less
        uint32_t W;
        uint32_t H;
        double   mean;
        double   expect;
    } test_case_t;

    // clang-format off
    test_case_t test_cases[] = {
        { .data = {100.0, 200.0, -200.0, 400.0, -300.0}, .W = 5, .H = 1, .mean = 40.0, .expect = 257.682},
        { .data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, .W = 10, .H = 1, .mean = 5.5, .expect = 2.87228},
        { .data = {
             1.0,  1.0,  1.0,  1.0, 
            -1.0, -1.0, -1.0, -1.0, 
             0.0,  0.0,  0.0,  0.0, 
            -1.0,  1.0, -1.0,  1.0
         }, .W = 4, .H = 4, .mean = 0, .expect = 0.866025},
        { .data = {10000.0, 20000.0, 30000.0, 40000.0, 50000.0, 60000.0, 70000.0, 80000.0, 90000.0, 100000.0}, .W = 10, .H = 1, .mean = 55000.0, .expect = 28722.813}
    };
    // clang-format on

    for (uint32_t i = 0; i < (sizeof(test_cases) / sizeof(test_case_t)); ++i) {
        double got = calculate_window_standard_deviation(
            test_cases[i].data,
            test_cases[i].W,
            test_cases[i].H,
            test_cases[i].mean
        );

        munit_assert_double_equal(test_cases[i].expect, got, 3);
    }

    return MUNIT_OK;
}

MunitResult test_extract_window(const MunitParameter params[], void* data) {
    (void)params;
    (void)data;

    const uint32_t W = 10;
    const uint32_t H = 10;
    // clang-format off
    double buf[] = {
        0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        1.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        2.0, 2.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        3.0, 3.0, 3.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        4.0, 4.0, 4.0, 4.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 7.0, 8.0, 9.0,
        7.0, 7.0, 7.0, 7.0, 7.0, 7.0, 7.0, 7.0, 8.0, 9.0,
        8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0, 9.0,
        9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0
    };
    // clang-format on

    double window_origin[3 * 3];
    // clang-format off
    double window_origin_check[3*3] = {
        0.0, 0.0, 1.0,
        0.0, 0.0, 1.0,
        1.0, 1.0, 1.0,
    };
    // clang-format on
    extract_window(buf, window_origin, 0, 0, 3, 3, W, H);
    munit_assert_memory_equal(
        3 * 3 * sizeof(double), window_origin_check, window_origin
    );

    double window1[5 * 5];
    // clang-format off
    double window1_check[5*5] = {
        0.0, 1.0, 2.0, 3.0, 4.0,
        1.0, 1.0, 2.0, 3.0, 4.0,
        2.0, 2.0, 2.0, 3.0, 4.0,
        3.0, 3.0, 3.0, 3.0, 4.0,
        4.0, 4.0, 4.0, 4.0, 4.0
    };
    // clang-format on
    extract_window(buf, window1, 2, 2, 5, 5, W, H);
    munit_assert_memory_equal(5 * 5 * sizeof(double), window1_check, window1);

    double window2[5 * 5];
    // clang-format off
    double window2_check[5*5] = {
        2.0, 3.0, 4.0, 5.0, 6.0,
        3.0, 3.0, 4.0, 5.0, 6.0,
        4.0, 4.0, 4.0, 5.0, 6.0,
        5.0, 5.0, 5.0, 5.0, 6.0,
        6.0, 6.0, 6.0, 6.0, 6.0
    };
    // clang-format on
    extract_window(buf, window2, 4, 4, 5, 5, W, H);
    munit_assert_memory_equal(5 * 5 * sizeof(double), window2_check, window2);

    double window_end[3 * 3];
    // clang-format off
    double window_end_check[3*3] = {
        8.0, 9.0, 9.0,
        9.0, 9.0, 9.0,
        9.0, 9.0, 9.0,
    };
    // clang-format on
    extract_window(buf, window_end, 9, 9, 3, 3, W, H);
    munit_assert_memory_equal(
        3 * 3 * sizeof(double), window_end_check, window_end
    );

    return MUNIT_OK;
}

MunitResult test_window_dot_product(const MunitParameter params[], void* data) {
    (void)params;
    (void)data;

    // clang-format off
    double left[] = {
        0.439743,  0.51252,  0.839973, 0.184437,
        0.746128,  0.183752, 0.587381, 0.220888,
        0.0459188, 0.748884, 0.606116, 0.411535,
        0.600958,  0.398145, 0.491724, 0.718804
    };

    double right[] = {
        0.504317,   0.00132375, 0.38332,   0.962383,
        0.951902,   0.513367,   0.0452651, 0.596563,
        0.00391566, 0.529286,   0.663347,  0.794245,
        0.320604,   0.400054,   0.686024,  0.227486
    };

    double expect = 3.66314;
    // clang-format on

    double got = window_dot_product(left, right, 4, 4);
    munit_assert_double_equal(expect, got, 4);

    return MUNIT_OK;
}

MunitResult test_zero_mean_window(const MunitParameter params[], void* data) {
    (void)params;
    (void)data;

    // clang-format off
    double window0[] = {
        0.117179, 0.804166, 0.956971, 0.0442088,
        0.374681, 0.595211, 0.547027, 0.815967,
        0.7144,   0.509105, 0.554394, 0.623328,
        0.53529,  0.43525,  0.556009, 0.377435
    };
    double mean0 = 0.53503875;
    double window0_expect[] = {
        -0.41786,  0.26913, 0.42193, -0.49083,
        -0.16036,  0.06017, 0.01199,  0.28093,
         0.17936, -0.02593, 0.01936,  0.08829,
         0.00025, -0.09979, 0.02097, -0.15760
    };
    // clang-format on

    zero_mean_window(window0, 4, 4, mean0);
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 4; ++x) {
            munit_assert_double_equal(
                window0_expect[(y * 4) + x], window0[(y * 4) + x], 4
            );
        }
    }

    // clang-format off
    double window1[] = {
        0.484439, 0.939595, 0.124232, 0.467126,
        0.924584, 0.658935, 0.290398, 0.0790969,
        0.381648, 0.298512, 0.35436,  0.936752,
        0.326236, 0.786972, 0.50629,  0.711116
    };
    double mean1 = 0.516893125;
    double window1_expect[] = {
        -0.03245,  0.42270, -0.39266, -0.04977,
         0.40769,  0.14204, -0.22650, -0.43780,
        -0.13525, -0.21838, -0.16253,  0.41986,
        -0.19066,  0.27008, -0.01060,  0.19422
    };
    // clang-format on
    zero_mean_window(window1, 4, 4, mean1);
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 4; ++x) {
            munit_assert_double_equal(
                window1_expect[(y * 4) + x], window1[(y * 4) + x], 4
            );
        }
    }

    return MUNIT_OK;
}

MunitResult test_normalize_window(const MunitParameter params[], void* data) {
    (void)params;
    (void)data;

    // clang-format off
    double window0[] = {
        -0.41786,  0.26913, 0.42193, -0.49083,
        -0.16036,  0.06017, 0.01199,  0.28093,
         0.17936, -0.02593, 0.01936,  0.08829,
         0.00025, -0.09979, 0.02097, -0.15760
    };
    double stddev0 = 0.23057347656085547;
    double window0_expect[] = {
        -1.81226,  1.16722, 1.82992, -2.12874,
        -0.69548,  0.26096, 0.05200,  1.21840,
         0.77789, -0.11246, 0.08396,  0.38291,
         0.00108, -0.43279, 0.09095, -0.68351
    };
    // clang-format on

    normalize_window(window0, 4, 4, stddev0);
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 4; ++x) {
            munit_assert_double_equal(
                window0_expect[(y * 4) + x], window0[(y * 4) + x], 4
            );
        }
    }

    // clang-format off
    double window1[] = {
        -0.03245,  0.42270, -0.39266, -0.04977,
         0.40769,  0.14204, -0.22650, -0.43780,
        -0.13525, -0.21838, -0.16253,  0.41986,
        -0.19066,  0.27008, -0.01060,  0.19422
    };
    double stddev1 = 0.27171798994933954;
    double window1_expect[] = {
        -0.11943,  1.55566, -1.44510, -0.18317,
         1.50042,  0.52275, -0.83358, -1.61123,
        -0.49776, -0.80370, -0.59816,  1.54521,
        -0.70168,  0.99397, -0.03901,  0.71479
    };
    // clang-format on
    normalize_window(window1, 4, 4, stddev1);
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 4; ++x) {
            munit_assert_double_equal(
                window1_expect[(y * 4) + x], window1[(y * 4) + x], 4
            );
        }
    }

    // clang-format off
    double window2[] = {
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0
    };
    double stddev2 = 0.0;
    double window2_expect[] = {
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0
    };
    // clang-format on
    normalize_window(window2, 4, 4, stddev2);
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 4; ++x) {
            munit_assert_double_equal(
                window2_expect[(y * 4) + x], window2[(y * 4) + x], 4
            );
        }
    }

    return MUNIT_OK;
}

MunitResult test_find_nearest_nonzero_neighbour(
    const MunitParameter params[], void* data
) {
    (void)params;
    (void)data;

    // clang-format off
    int32_t map[] = {
        0, 0, 0, 3, 4, 5, 6, 7, 8, 9,
        0, 0, 0, 3, 4, 5, 6, 7, 8, 9,
        2, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        3, 1, 2, 0, 0, 0, 0, 0, 8, 9,
        4, 1, 2, 0, 0, 0, 0, 0, 8, 9,
        5, 1, 2, 0, 0, 0, 0, 7, 8, 9,
        6, 1, 2, 0, 0, 0, 0, 0, 8, 9,
        7, 1, 2, 0, 0, 0, 0, 0, 8, 9,
        8, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        9, 1, 2, 3, 4, 5, 6, 7, 8, 9
    };
    // clang-format on

    int32_t got0 =
        find_nearest_nonzero_neighbour(map, 10, 10, 0, 0, NULL, NULL);
    int32_t want0 = 2;
    munit_assert_int32(want0, ==, got0);

    int32_t got1 =
        find_nearest_nonzero_neighbour(map, 10, 10, 5, 5, NULL, NULL);
    int32_t want1 = 7;
    munit_assert_int32(want1, ==, got1);

    int32_t got2 =
        find_nearest_nonzero_neighbour(map, 10, 10, 2, 2, NULL, NULL);
    int32_t want2 = 2;
    munit_assert_int32(want2, ==, got2);

    // out of bounds check
    int32_t got3 =
        find_nearest_nonzero_neighbour(map, 10, 10, 15, 15, NULL, NULL);
    int32_t want3 = 0;
    munit_assert_int32(want3, ==, got3);

    // clang-format off
    int32_t empty_map[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    // clang-format on

    int32_t got_empty =
        find_nearest_nonzero_neighbour(empty_map, 10, 10, 5, 5, NULL, NULL);
    int32_t want_empty = 0;
    munit_assert_int32(want_empty, ==, got_empty);

    return MUNIT_OK;
}

MunitResult test_find_nearest_nonzero_neighbour_prealloc(
    const MunitParameter params[], void* data
) {
    (void)params;
    (void)data;

    uint8_t*     visited = malloc(10 * 10 * sizeof(uint8_t));
    coord_fifo_t fifo    = {
           .storage  = malloc(sizeof(coord_t) * 10 * 10),
           .read     = 0,
           .write    = 0,
           .capacity = 10 * 10
    };

    // clang-format off
    int32_t map[] = {
        0, 0, 0, 3, 4, 5, 6, 7, 8, 9,
        0, 0, 0, 3, 4, 5, 6, 7, 8, 9,
        2, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        3, 1, 2, 0, 0, 0, 0, 0, 8, 9,
        4, 1, 2, 0, 0, 0, 0, 0, 8, 9,
        5, 1, 2, 0, 0, 0, 0, 7, 8, 9,
        6, 1, 2, 0, 0, 0, 0, 0, 8, 9,
        7, 1, 2, 0, 0, 0, 0, 0, 8, 9,
        8, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        9, 1, 2, 3, 4, 5, 6, 7, 8, 9
    };
    // clang-format on

    int32_t got0 =
        find_nearest_nonzero_neighbour(map, 10, 10, 0, 0, visited, &fifo);
    int32_t want0 = 2;
    munit_assert_int32(want0, ==, got0);

    int32_t got1 =
        find_nearest_nonzero_neighbour(map, 10, 10, 5, 5, visited, &fifo);
    int32_t want1 = 7;
    munit_assert_int32(want1, ==, got1);

    int32_t got2 =
        find_nearest_nonzero_neighbour(map, 10, 10, 2, 2, visited, &fifo);
    int32_t want2 = 2;
    munit_assert_int32(want2, ==, got2);

    // out of bounds check
    int32_t got3 =
        find_nearest_nonzero_neighbour(map, 10, 10, 15, 15, visited, &fifo);
    int32_t want3 = 0;
    munit_assert_int32(want3, ==, got3);

    // clang-format off
    int32_t empty_map[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    // clang-format on

    int32_t got_empty =
        find_nearest_nonzero_neighbour(empty_map, 10, 10, 5, 5, visited, &fifo);
    int32_t want_empty = 0;
    munit_assert_int32(want_empty, ==, got_empty);

    free(visited);
    free(fifo.storage);

    return MUNIT_OK;
}

MunitResult test_coord_fifo(const MunitParameter params[], void* data) {
    (void)params;
    (void)data;

    munit_assert_null(coord_fifo_dequeue(NULL));
    munit_assert_int32(-1, ==, coord_fifo_enqueue(NULL, (coord_t){0, 0}));
    munit_assert_int32(0, ==, coord_fifo_len(NULL));

    coord_fifo_t fifo = {
        .storage  = malloc(sizeof(coord_t) * 8),
        .read     = 0,
        .write    = 0,
        .capacity = 9
    };

    munit_assert_uint32(0, ==, coord_fifo_len(&fifo));
    munit_assert_null(coord_fifo_dequeue(&fifo));
    munit_assert_null(coord_fifo_dequeue(&fifo));
    munit_assert_uint32(0, ==, coord_fifo_len(&fifo));

    for (uint32_t i = 0; i < 8; ++i) {
        munit_logf(MUNIT_LOG_INFO, "enqueue idx %d", i);
        int32_t enq = coord_fifo_enqueue(&fifo, (coord_t){0, 0});
        munit_assert_int32(0, ==, enq);
        munit_assert_uint32(i + 1, ==, coord_fifo_len(&fifo));
    }
    int32_t enq_fail = coord_fifo_enqueue(&fifo, (coord_t){0, 0});
    munit_assert_int32(-1, ==, enq_fail);

    for (uint32_t i = 0; i < 8; ++i) {
        munit_logf(MUNIT_LOG_INFO, "dequeue idx %d", i);
        munit_assert_not_null(coord_fifo_dequeue(&fifo));
        munit_assert_uint32(7 - i, ==, coord_fifo_len(&fifo));
    }
    munit_assert_null(coord_fifo_dequeue(&fifo));

    free(fifo.storage);

    return MUNIT_OK;
}

#define TEST_IMAGE_PATH_0 "./test_images/input/im0.png"
#define TEST_IMAGE_PATH_1 "./test_images/input/im1.png"

MunitResult test_image_loading(const MunitParameter params[], void* data) {
    (void)params;
    (void)data;

    {
        // load im0.png
        img_load_result_t expect_0 = {
            .err = 0, .img_desc.height = 2016, .img_desc.width = 2940
        };

        img_load_result_t got_0;
        load_image(TEST_IMAGE_PATH_0, &got_0);

        munit_assert_uint32(expect_0.img_desc.width, ==, got_0.img_desc.width);
        munit_assert_uint32(
            expect_0.img_desc.height, ==, got_0.img_desc.height
        );
        munit_assert_uint(0, ==, got_0.err);
        munit_assert_not_null(got_0.img_desc.img);

        free(got_0.img_desc.img);
    }
    {
        // load im1.png
        img_load_result_t expect_1 = {
            .err = 0, .img_desc.height = 2016, .img_desc.width = 2940
        };

        img_load_result_t got_1;
        load_image(TEST_IMAGE_PATH_1, &got_1);

        munit_assert_uint32(expect_1.img_desc.width, ==, got_1.img_desc.width);
        munit_assert_uint32(
            expect_1.img_desc.height, ==, got_1.img_desc.height
        );
        munit_assert_uint(0, ==, got_1.err);
        munit_assert_not_null(got_1.img_desc.img);

        free(got_1.img_desc.img);
    }

    return MUNIT_OK;
}

bool images_equal(const char* a, const char* b) {
    FILE* af = fopen(a, "rb");
    FILE* bf = fopen(b, "rb");

    munit_assert_not_null(af);
    munit_assert_not_null(bf);

    char ac = 0;
    char bc = 0;

    while (true) {
        ac = getc(af);
        bc = getc(bf);

        if (ac != bc) {
            fclose(af);
            fclose(bf);
            return false;
        }

        if (ac == EOF) {
            // also bc == EOF
            break;
        }
    }

    fclose(af);
    fclose(bf);
    return true;
}

MunitResult test_output_image(const MunitParameter params[], void* data) {
    (void)params;
    (void)data;

    const uint32_t W = 100;
    const uint32_t H = 100;

    {
        // output GS image
        const char* reference_image = "./test_images/output/reference_gs_0.png";
        const char* generated_image =
            "./test_images/output/test_generated_gs_0.png";

        gray_img_t test_image = {
            .img    = malloc(W * H * sizeof(gray_t)),
            .width  = W,
            .height = H,
        };
        munit_assert_not_null(test_image.img);

        // generate 100x100 image containing some pattern
        for (uint32_t y = 0; y < H; ++y) {
            for (uint32_t x = 0; x < W; ++x) {
                test_image.img[y * W + x] = (gray_t)(x ^ y);
            }
        }

        img_write_result_t write_result;

        output_image(generated_image, &test_image, GS, &write_result);

        munit_assert(write_result.err == 0);

        // check reference image and generated image are equal
        munit_assert_true(images_equal(reference_image, generated_image));
    }
    {
        // output RGBA image
        const char* reference_image =
            "./test_images/output/reference_rgba_0.png";
        const char* generated_image =
            "./test_images/output/test_generated_rgba_0.png";

        rgba_img_t test_image = {
            .img    = malloc(W * H * sizeof(rgba_t)),
            .width  = W,
            .height = H,
        };
        munit_assert_not_null(test_image.img);

        // generate 100x100 image containing some pattern
        for (uint32_t y = 0; y < H; ++y) {
            for (uint32_t x = 0; x < W; ++x) {
                test_image.img[y * W + x].R = 255 * !(x & y);
                test_image.img[y * W + x].G = x ^ y;
                test_image.img[y * W + x].B = x | y;
                test_image.img[y * W + x].A = 255;
            }
        }

        img_write_result_t write_result;

        output_image(generated_image, &test_image, RGBA, &write_result);

        munit_assert(write_result.err == 0);

        // check reference image and generated image are equal
        munit_assert_true(images_equal(reference_image, generated_image));
    }
    {
        // output GS float image
        // TODO: move `output_grayscale_float_image` func from phase_4/main.c
        // into src/image_operations.c to enable testing it
    }
    {
        // output GS int32 image
        // TODO: move `output_grayscale_int32_image` func from phase_4/main.c
        // into src/image_operations.c to enable testing it
    }

    return MUNIT_OK;
}

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
    // clang-format off
    static MunitTest tests[] = {
        {
            "mean",
            test_calculate_window_mean,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "standard_deviation",
            test_calculate_window_standard_deviation,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "extract_window",
            test_extract_window,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "window_dot_product",
            test_window_dot_product,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "zero_mean_window",
            test_zero_mean_window,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "normalize_window",
            test_normalize_window,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "coord_fifo",
            test_coord_fifo,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "find_nearest_nonzero_neighbour",
            test_find_nearest_nonzero_neighbour,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "find_nearest_nonzero_neighbour_prealloc",
            test_find_nearest_nonzero_neighbour_prealloc,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "test_image_loading",
            test_image_loading,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {
            "test_output_image",
            test_output_image,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        },
        {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}
    };
    // clang-format on

    static MunitSuite test_suite = {
        "zncc_operations/", &tests[0], NULL, 1, MUNIT_SUITE_OPTION_NONE
    };

    return munit_suite_main(&test_suite, (void*)"", argc, argv);
}