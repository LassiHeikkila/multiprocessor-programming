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

    const unsigned int l_x = (Wi / N) * i;
    const unsigned int h_x = (Wi / N) * (i + 1);

    const unsigned int l_y = (Hi / N) * j;
    const unsigned int h_y = (Hi / N) * (j + 1);

    const unsigned int s_x = Wi / Wo;
    const unsigned int s_y = Hi / Ho;

    unsigned int yo = 0;
    unsigned int yi = 0;
    unsigned int xo = 0;
    unsigned int xi = 0;

    for (yi = l_y; yi < h_y; yi += s_y) {
        yo = yi / s_y;
        for (xi = l_x; xi < h_x; xi += s_x) {
            xo = xi / s_x;

            out[(yo * Wo) + xo] = in[(yi * Wi) + xi];
        }
    }
}