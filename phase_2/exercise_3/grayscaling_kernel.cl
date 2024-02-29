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

    const unsigned int mw = W % N;              // modulo width
    const unsigned int mh = H % N;              // modulo height
    const unsigned int sw = (W - mw) / N;       // segment width
    const unsigned int sh = (H - mh) / N;       // segment height
    const unsigned int lx = sw * i;             // low x
    const unsigned int hx = sw * (i + 1) + ((i == (N-1)) ? mw : 0); // high x
    const unsigned int ly = sh * j;             // low y
    const unsigned int hy = sh * (j + 1) + ((j == (N-1)) ? mh : 0); // high y

    unsigned int y = 0;
    unsigned int x = 0;
    uchar4 px_in = (uchar4)(0, 0, 0, 0);
    float4 px = (float4)(0.0f, 0.0f, 0.0f, 0.0f);

    for (y = ly; y < hy; ++y) {
        for (x = lx; x < hx; ++x) {
            px_in = in[(y*W)+x];
            px = convert_float4(px_in) * grayscaling_factors;

            out[(y*W) + x] = (unsigned char)(px.x+ px.y + px.z);
        }
    }
}