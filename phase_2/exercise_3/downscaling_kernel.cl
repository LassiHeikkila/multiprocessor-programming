__kernel void downscale_kernel(
    const int N,
    const int Wi,
    const int Hi,
    const int Wo,
    const int Ho,
    __global unsigned int *in,
    __global unsigned int *out
) {
    // N is number of sections the overall image is divided into.
    // i,j determine the coordinates of the section the kernel should operate on

    // Wi is width of input image
    // Hi is height of input image
    // Wo is width of output image
    // Ho is height of output image

    int i = get_global_id(0);
    int j = get_global_id(1);

    const int l_x = (Wi / N) * i;
    const int h_x = (Wi / N) * (i + 1);

    const int l_y = (Hi / N) * j;
    const int h_y = (Hi / N) * (j + 1);

    const int s_x = Wi / Wo;
    const int s_y = Hi / Ho;

    unsigned int yo = 0;
    unsigned int yi = 0;
    unsigned int xo = 0;
    unsigned int xi = 0;

    for (yi = l_y; yi < h_y; yi += s_y) {
        yo = yi / s_y;
        for (xi = l_x; xi < h_x; xi += s_x) {
            xo = xi / s_x;

            out[(yo*Wo) + xo] = in[(yi*Wi)+xi];
        }
    }
}