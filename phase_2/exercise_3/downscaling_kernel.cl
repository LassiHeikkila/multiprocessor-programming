__kernel void downscale_kernel(
    const unsigned int N,
    const unsigned int Wi,
    const unsigned int Hi,
    const unsigned int Wo,
    const unsigned int Ho,
    __global unsigned int *in,
    __global unsigned int *out
) {
    // N is number of sections the overall image is divided into.
    // i,j determine the coordinates of the section the kernel should operate on

    // Wi is width of input image
    // Hi is height of input image
    // Wo is width of output image
    // Ho is height of output image

    const unsigned int i = get_global_id(0);
    const unsigned int j = get_global_id(1);

    const unsigned int mw = Wo % N;             // modulo width
    const unsigned int mh = Ho % N;             // modulo height
    const unsigned int sw = (Wo - mw) / N;      // segment width
    const unsigned int sh = (Ho - mh) / N;      // segment height
    const unsigned int lx = sw * i;             // low x
    const unsigned int hx = sw * (i + 1) + ((i == (N-1)) ? mw : 0); // high x
    const unsigned int ly = sh * j;             // low y
    const unsigned int hy = sh * (j + 1) + ((j == (N-1)) ? mh : 0); // high y
    const unsigned int sx = Wi / Wo;            // scale x
    const unsigned int sy = Hi / Ho;            // scale y

    unsigned int yo = 0;
    unsigned int yi = 0;
    unsigned int xo = 0;
    unsigned int xi = 0;

    unsigned int sz = Wo*Ho;
    unsigned int offset = 0;

    for (yo = ly; yo < hy; ++yo) {
        yi = yo*sy;
        for (xo = lx; xo < hx; ++xo) {
            xi = xo*sx;

            out[(yo*Wo) + xo] = in[(yi*Wi)+xi];
        }
    }

}