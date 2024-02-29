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
    const unsigned int A = ((2*R)+1)*((2*R)+1); // A for area, "R*R"

    const unsigned int i = get_global_id(0);
    const unsigned int j = get_global_id(1);

    const unsigned int mw = W % N;              // modulo width
    const unsigned int mh = H % N;              // modulo height
    const unsigned int sw = (W - mw) / N;       // segment width
    const unsigned int sh = (H - mh) / N;       // segment height
    const unsigned int lx = sw * i;             // low x
    const unsigned int hx = sw * (i + 1) + ((i == (N-1)) ? mw : 0); // high x
    const unsigned int ly = sh * j;             // low y
    const unsigned int hy = sh * (j + 1) + ((j == (N-1)) ? mh : 0); // high y

    int y = 0;
    int x = 0;
    int dy = 0;
    int dx = 0;
    int py = 0; // final position y
    int px = 0; // final position x
    unsigned int v = 0; // accumulator which is wider than inputs to avoid overflow

    for (y = ly; y < hy; ++y) {
        for (x = lx; x < hx; ++x) {
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