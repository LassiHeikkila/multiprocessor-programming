const float4 grayscaling_factors = (float4)(0.2126f, 0.7152f, 0.0722f, 0.0f);

__kernel void grayscale_kernel(
    const unsigned int N,
    const unsigned int W,
    const unsigned int H,
    read_only image2d_t in,
    write_only image2d_t out
) {
    // N is number of sections the overall image is divided into.
    // i determines the rows of the section the kernel should operate on

    // W is width
    // H is height

    const unsigned int i = get_global_id(0);
    
    const unsigned int mw = W % N;              // modulo width
    const unsigned int mh = H % N;              // modulo height
    const unsigned int sw = (W - mw) / N;       // segment width
    const unsigned int sh = (H - mh) / N;       // segment height
    const unsigned int ly = sh * i;             // low y
    const unsigned int hy = sh * (i + 1) + ((i == (N-1)) ? mh : 0); // high y

    unsigned int y = 0;
    unsigned int x = 0;
    uchar4 px_in = (uchar4)(0, 0, 0, 0);
    float4 px_out = (float4)(0.0f, 0.0f, 0.0f, 0.0f);

    const sampler_t s = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

    for (y = ly; y < hy; ++y) {
        for (x = 0; x < W; ++x) {
            int2 coord = (x, y);
            uint4 px_in = read_imageui(in, s, coord);

            px_out = convert_float4(px_in) * grayscaling_factors;

            write_imagef(out, coord, (px_out.x + px_out.y + px_out.z));
        }
    }
}