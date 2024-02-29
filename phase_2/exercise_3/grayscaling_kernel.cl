const float4 grayscaling_factors = (float4)(0.2126f, 0.7152f, 0.0722f, 0.0f);

__kernel void grayscale_kernel(
    const unsigned int N,
    const unsigned int W,
    const unsigned int H,
    __global uchar4 *in,
    __global unsigned char *out
) {
    // N is number of sections the overall image is divided into.
    // i,j determine the coordinates of the section the kernel should operate on

    // W is width
    // H is height

    const unsigned int i = get_global_id(0);
    const unsigned int j = get_global_id(1);

    const unsigned int l_x = (W / (N)) * i;
    const unsigned int h_x = (W / (N)) * (i + 1);

    const unsigned int l_y = (H / (N)) * j;
    const unsigned int h_y = (H / (N)) * (j + 1);

    unsigned int y = 0;
    unsigned int x = 0;
    uchar4 px_in = (uchar4)(0, 0, 0, 0);
    float4 px = (float4)(0.0f, 0.0f, 0.0f, 0.0f);

    for (y = l_y; y < h_y; ++y) {
        for (x = l_x; x < h_x; ++x) {
            px_in = in[(y*W)+x];
            px = convert_float4(px_in) * grayscaling_factors;

            out[(y*W) + x] = (unsigned char)(px.x+ px.y + px.z);
        }
    }
}