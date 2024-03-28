#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH 9u
#endif

#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT 9u
#endif

#ifndef CROSSCHECK_THRESHOLD
#define CROSSCHECK_THRESHOLD 24
#endif

#define WINDOW_SIZE (WINDOW_HEIGHT * WINDOW_WIDTH)

void extract_window(
    const int2 offset,
    const int2 input_dimensions,
    read_only image2d_t img,
    __private float* window
) {
    // assumes that out has capacity for (window_dimensions.x * window_dimensions.y) floats

    int2 coord = (0, 0);
    unsigned int out_idx = 0;

    int x = 0;
    int y = 0;

    const sampler_t s = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

    for (
        y = offset.y - ((WINDOW_HEIGHT - 1) / 2);
        y <= offset.y + ((WINDOW_HEIGHT -1) / 2);
        ++y
    ) {
        coord.y = clamp(y, 0, input_dimensions.y);
        for (
            x = offset.x - ((WINDOW_WIDTH - 1) / 2);
            x <= offset.x + ((WINDOW_WIDTH - 1) / 2);
            ++x
        ) {
            coord.x = clamp(x, 0, input_dimensions.x);

            float4 px = read_imagef(img, s, coord);
            window[out_idx] = px.x;

            out_idx += 1;
        }
    }
}

float calculate_window_mean(
    __private float *window
) {
    float total = 0.0f;
    for (unsigned int i = 0; i < WINDOW_SIZE; ++i) {
        total += window[i];
    }
    return total / (float)WINDOW_SIZE;
}

float calculate_window_standard_deviation(
    __private float *window,
    float mean
) {
    float d = 0.0f;
    float variance_sum = 0.0f;
    float sigma = 0.0f;

    for (unsigned int i = 0; i < WINDOW_SIZE; ++i) {
        d = window[i] - mean;
        variance_sum += d * d;
    }

    sigma = sqrt(variance_sum / (float)WINDOW_SIZE);

    return sigma;
}

void zero_mean_window(
    __private float *window,
    float mean
) {
    for (unsigned int i = 0; i < WINDOW_SIZE; ++i) {
        window[i] = window[i] - mean;
    }
}

void normalize_window(
    __private float *window,
    float sigma
) {
    if (fabs(sigma) <= 1e-4) {
        return;
    }

    for (unsigned int i = 0; i < WINDOW_SIZE; ++i) {
        window[i] = window[i] / sigma;
    }
}

float window_dot_product(
    __private float *a,
    __private float* b
) {
    float sum = 0.0f;

    for (unsigned int i = 0; i < WINDOW_SIZE; ++i) {
        sum += a[i] * b[i];
    }

    return sum;
}

void extract_normalized_window(
    const int2 offset,
    const int2 input_dimensions,
    read_only image2d_t img,
    __private float* window
) {
    extract_window(offset, input_dimensions, img, window);
    float mu = calculate_window_mean(window);
    float sigma = calculate_window_standard_deviation(window, mu);
    zero_mean_window(window, mu);
    normalize_window(window, sigma);
}

__kernel void calculate_zncc(
    const unsigned int N,
    const unsigned int M,
    const unsigned int W,
    const unsigned int H,
    const int direction, // negative for right to left, positive for left to right
    const unsigned int max_disparity,
    read_only image2d_t img_left,
    read_only image2d_t img_right,
    __global int *out
) {
    if (direction == 0) {
        printf("ERROR: direction set to 0. It should be negative for right to left processing, or positive for left to right processing.");
        return;
    }

    // N is number of sections the image height is divided into.
    // i determines the rows of the section the kernel should operate on

    // M is the number of sections the image width is divided into
    // j determines the columns of the section the kernel should operate on

    // W is image width
    // H is image height

    const unsigned int i = get_global_id(0);
    const unsigned int j = get_global_id(1);

    const unsigned int mh = H % N;        // modulo height
    const unsigned int sh = (H - mh) / N; // segment height
    const unsigned int ly = sh * i;         // low y
    const unsigned int hy = sh * (i + 1) + ((i == (N-1)) ? mh : 0); // high y

    const unsigned int mw = W % M;        // modulo width
    const unsigned int sw = (W - mw) / M; // segment width
    const unsigned int lx = sw * j;         // low x
    const unsigned int hx = sw * (j + 1) + ((j == (M-1)) ? mw : 0); // high x

    unsigned int y = 0;
    unsigned int x = 0;

    __private float window_left[WINDOW_SIZE];
    __private float window_right[WINDOW_SIZE];
    
    if (direction < 0) {
        // right to left
        for (y = ly; y < hy; ++y) {
            for (x = lx; x < hx; ++x) {
                float max_sum = 0.0f;
                int best_disparity = 0;

                extract_normalized_window((x, y), (H, W), img_right, window_right);

                for (int d = 0; d < min(W - x - 1, max_disparity); ++d) {
                    extract_normalized_window((x + d, y), (H, W), img_left, window_left);

                    float zncc = window_dot_product(window_left, window_right);

                    if (zncc > max_sum) {
                        max_sum = zncc;
                        best_disparity = d;
                    }
                }

                out[(y * W) + x] = best_disparity;
            }
        }
    } else {
        // left to right

        for (y = ly; y < hy; ++y) {
            for (x = lx; x < hx; ++x) {
                float max_sum = 0.0f;
                int best_disparity = 0;

                extract_normalized_window((x, y), (H, W), img_left, window_left);

                for (int d = 0; d < min(x, max_disparity); ++d) {
                    extract_normalized_window((x - d, y), (H, W), img_right, window_right);

                    float zncc = window_dot_product(window_left, window_right);

                    if (zncc > max_sum) {
                        max_sum = zncc;
                        best_disparity = d;
                    }
                }

                out[(y * W) + x] = best_disparity;
            }
        }
    }
}


__kernel void cross_check(
    const unsigned int N,
    const unsigned int W,
    const unsigned int H,
    __global int *left,
    __global int *right,
    __global int *out
) {
    // N is number of sections the image height is divided into.
    // i determines the rows of the section the kernel should operate on

    // W is image width
    // H is image height

    const unsigned int i = get_global_id(0);

    const unsigned int mh = H % N;        // modulo height
    const unsigned int sh = (H - mh) / N; // segment height
    const unsigned int ly = sh * i;       // low y
    const unsigned int hy = sh * (i + 1) + ((i == (N-1)) ? mh : 0); // high y

    for (unsigned int y = ly; y < hy; ++y) {
        for (unsigned int x = 0; x < W; ++x) {
            unsigned int offset = ((y * W) + x);
            int disparity = left[offset];
            int delta = abs(disparity - right[offset - disparity]);

            if (delta < CROSSCHECK_THRESHOLD) {
                out[offset] = disparity;
            } else {
                out[offset] = 0;
            }
        }
    }
}

__kernel void fill_zero_regions(
    const unsigned int N,
    const unsigned int W,
    const unsigned int H,
    __global int *d
) {}