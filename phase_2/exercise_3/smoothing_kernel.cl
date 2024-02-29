__kernel void smoothing_kernel(
    const unsigned int N,
    const unsigned int W,
    const unsigned int H,
    const int R,
    __global unsigned char *in,
    __global unsigned char *out
) {
    // N is number of sections the overall image is divided into.
    // i,j determine the coordinates of the section the kernel should operate on

    // W is width
    // H is height
    // R is radius of smoothing mask, e.g. for 5x5 filter it is 2
    const unsigned int A = (R+1)*(R+1); // A for area, "R*R"

    const unsigned int i = get_global_id(0);
    const unsigned int j = get_global_id(1);

    const int l_x = (W / (N)) * i;
    const int h_x = (W / (N)) * (i + 1);

    const int l_y = (H / (N)) * j;
    const int h_y = (H / (N)) * (j + 1);

    int y = 0;
    int x = 0;
    int dy = 0;
    int dx = 0;
    int py = 0; // final position y
    int px = 0; // final position x
    unsigned int v = 0; // accumulator which is wider than inputs to avoid overflow

    for (y = l_y; y < h_y; ++y) {
        for (x = l_x; x < h_x; ++x) {
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
            out[(y*W) + x] = (unsigned char)(v / (A));
        }
    }
}