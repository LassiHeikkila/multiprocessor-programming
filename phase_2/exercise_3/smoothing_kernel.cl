__kernel void smoothing_kernel(
    const int N,
    const int W,
    const int H,
    const unsigned int R,
    __global unsigned char *in,
    __global unsigned char *out
) {
    // N is number of sections the overall image is divided into.
    // i,j determine the coordinates of the section the kernel should operate on

    // W is width
    // H is height
    // R is radius of smoothing mask, e.g. for 5x5 filter it is 2

    int i = get_global_id(0);
    int j = get_global_id(1);

    const int l_x = (W / N) * i;
    const int h_x = (W / N) * (i + 1);

    const int l_y = (H / N) * j;
    const int h_y = (H / N) * (j + 1);

    unsigned int y = 0;
    unsigned int x = 0;
    int dy = 0;
    int dx = 0;
    unsigned int py = 0; // final position y
    unsigned int px = 0; // final position x
    unsigned int v = 0; // accumulator

    for (y = l_y; y < h_y; ++y) {
        for (x = l_x; y < h_x; ++x) {
            v = 0;
            for (dy = -R; dy <= R; ++dy) {
                py = y + dy;
                if (py < 0) {
                    py = 0;
                } else if (py >= H) {
                    py = H - 1;
                }

                for (dx = -R; dx <= R; ++dx) {
                    px = x + dx;
                    if (px < 0) {
                        px = 0;
                    } else if (px >= W) {
                        px = W - 1;
                    }

                    v += (unsigned int)in[(py*W)+px];
                }
            }
            out[(y*W) + x] = (unsigned char)(v / (R*R));
        }
    }
}