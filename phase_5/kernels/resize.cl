__kernel void downscale_kernel(
    const unsigned int N,
    const unsigned int Wi,
    const unsigned int Hi,
    const unsigned int Wo,
    const unsigned int Ho,
    read_only image2d_t in,
    write_only image2d_t out
) {
    // N is number of sections the overall image is divided into.
    // i determines the rows of the section the kernel should operate on

    // Wi is width of input image
    // Hi is height of input image
    // Wo is width of output image
    // Ho is height of output image

    const unsigned int i = get_global_id(0);

    const unsigned int mw = Wo % N;             // modulo width
    const unsigned int mh = Ho % N;             // modulo height
    const unsigned int sw = (Wo - mw) / N;      // segment width
    const unsigned int sh = (Ho - mh) / N;      // segment height
    const unsigned int ly = sh * i;             // low y
    const unsigned int hy = sh * (i + 1) + ((i == (N-1)) ? mh : 0); // high y
    const unsigned int sx = Wi / Wo;            // scale x
    const unsigned int sy = Hi / Ho;            // scale y

    unsigned int yo = 0;
    unsigned int yi = 0;
    unsigned int xo = 0;
    unsigned int xi = 0;

    unsigned int sz = Wo*Ho;
    unsigned int offset = 0;

    const sampler_t s = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

    for (yo = ly; yo < hy; ++yo) {
        yi = yo*sy;
        for (xo = 0; xo < Wo; ++xo) {
            xi = xo*sx;

            int2 coord_in = (xi, yi);
            int2 coord_out = (xo, yo);
            uint4 px = read_imageui(in, s, coord_in);
            write_imageui(out, coord_out, px);
        }
    }

}