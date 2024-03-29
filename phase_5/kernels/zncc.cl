#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH 9u
#endif

#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT 9u
#endif

#ifndef CROSSCHECK_THRESHOLD
#define CROSSCHECK_THRESHOLD 8
#endif

#define WINDOW_SIZE (WINDOW_HEIGHT * WINDOW_WIDTH)

void extract_window(
    const int2 offset,
    const int2 input_dimensions,
    read_only image2d_t img,
    __private float* window
) {
    // assumes that out has capacity for (window_dimensions.x * window_dimensions.y) floats

    int2 coord = (int2)(0, 0);
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

    // both have same dimensions
    const int W = get_image_width(img_left);
    const int H = get_image_height(img_left);

    const int i = get_global_id(0);

    const int mh = H % N;        // modulo height
    const int sh = (H - mh) / N; // segment height
    const int ly = sh * i;       // low y
    const int hy = sh * (i + 1) + ((i == (N-1)) ? mh : 0); // high y

    const int2 image_dimensions = (int2)(W, H);

    __private float window_left[WINDOW_SIZE];
    __private float window_right[WINDOW_SIZE];

    if (direction < 0) {
        // right to left
        for (int y = ly; y < hy; ++y) {
            for (int x = 0; x < W; ++x) {
                float max_sum = 0.0f;
                int best_disparity = 0;

                int2 coord_r = (int2)(x, y);
                extract_normalized_window(coord_r, image_dimensions, img_right, window_right);

                for (int d = 0; d < min(W - x, (int)max_disparity); ++d) {
                    int2 coord_l = (int2)(x + d, y);

                    extract_normalized_window(coord_l, image_dimensions, img_left, window_left);

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
        for (int y = ly; y < hy; ++y) {
            for (int x = 0; x < W; ++x) {
                float max_sum = 0.0f;
                int best_disparity = 0;

                int2 coord_l = (int2)(x, y);

                extract_normalized_window(coord_l, image_dimensions, img_left, window_left);

                for (int d = 0; d < min(x, (int)max_disparity); ++d) {
                    int2 coord_r = (int2)(x - d, y);
                    extract_normalized_window(coord_r, image_dimensions, img_right, window_right);

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

int find_horizontal_linear_avg_to_nonzero_neighbours(__global int *d, int2 coord, int2 dim) {
    int2 left = {coord.x, 0}; // x is x, s1 is value
    int2 right = {coord.x, 0}; // x is x, s1 is value

    // find left
    while (left.s1 == 0) {
        left.x -= 1;
        left.s1 = d[(coord.y * dim.x) + left.x];
        if (left.x <= 0) {
            break;
        }
    }

    // find right
    while (right.s1 == 0) {
        left.x += 1;
        left.s1 = d[(coord.y * dim.x) + right.x];
        if (right.x >= dim.x) {
            break;
        }
    }

    if (left.s1 == 0 && right.s1 == 0) {
        return 0;
    }

    if (left.s1 == 0 && right.s1 != 0) {
        return right.s1;
    }

    if (left.s1 != 0 && right.s1 == 0) {
        return left.s1;
    }
    
    
    // linear interpolation
    return ((left.x * right.s1) + (right.x * left.s1) / (right.x - left.x));
}

int find_vertical_linear_avg_to_nonzero_neighbours(__global int *d, int2 coord, int2 dim) {
    int2 top = {0, coord.y}; // y is y, s0 is value
    int2 bottom = {0, coord.y}; // y is y, s0 is value

    // find top
    while (top.s0 == 0) {
        top.y -= 1;
        top.s0 = d[(top.y * dim.x) + coord.x];
        if (top.y <= 0) {
            break;
        }
    }
    
    // find bottom
    while(bottom.s0 == 0) {
        top.y += 1;
        top.s0 = d[(top.y * dim.x) + coord.x];
        if (top.y >= dim.y) {
            break;
        }
    }

    if (top.s0 == 0 && bottom.s0 == 0) {
        return 0;
    }

    if (top.s0 == 0 && bottom.s0 != 0) {
        return bottom.s0;
    }

    if (top.s0 != 0 && bottom.s0 == 0) {
        return top.s0;
    }
    
    
    // linear interpolation
    return ((top.y * bottom.s0) + (bottom.y * top.s0) / (bottom.y - top.y));
}

__kernel void fill_zero_regions(
    const unsigned int N,
    const unsigned int W,
    const unsigned int H,
    __global int *d
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

    // do linear interpolation along x and y axes, don't need any FIFO etc for that.

    for (unsigned int y = ly; y < hy; ++y) {
        for (unsigned int x = 0; x < W; ++x) {
            int v = d[(y*W)+x];
            if (v == 0) {
                int ha = find_horizontal_linear_avg_to_nonzero_neighbours(d, (int2)(x, y), (int2)(W, H));
                int va = find_vertical_linear_avg_to_nonzero_neighbours(d, (int2)(x, y), (int2)(W, H));

                d[(y * W) + x] = (ha + va) / 2;
            }
        }
    }
}