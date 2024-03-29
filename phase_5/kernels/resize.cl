__kernel void downscale_kernel(
    const unsigned int N,
    read_only image2d_t in,
    write_only image2d_t out
) {
    // N is number of sections the overall image is divided into.
    // i determines the rows of the section the kernel should operate on

    // Wi is width of input image
    // Hi is height of input image
    // Wo is width of output image
    // Ho is height of output image

    const int Wi = get_image_width(in);
    const int Hi = get_image_height(in);

    const int Wo = get_image_width(out);
    const int Ho = get_image_height(out);

    const int i = get_global_id(0);

    const int mh = Ho % N;             // modulo height
    const int sh = (Ho - mh) / N;      // segment height
    const int ly = sh * i;             // low y
    const int hy = sh * (i + 1) + ((i == (N-1)) ? mh : 0); // high y

    const int sy = Hi / Ho;
    const int sx = Wi / Wo;
    const int2 sc = {(int)(sx), (int)(sy)};

    int xo = 0;
    int yo = 0;

    for (yo = ly; yo < hy; ++yo) {
        for (xo = 0; xo < Wo; ++xo) {
            int2 coord_out = {xo, yo};
            int2 coord_in = coord_out * sc;
            uint4 px = read_imageui(in, coord_in);
            write_imageui(out, coord_out, px);
        }
    }
}