const float4 grayscaling_factors = (float4)(0.2126f, 0.7152f, 0.0722f, 0.0f);

__kernel void grayscale_kernel(
    const unsigned int N,
    read_only image2d_t in,
    write_only image2d_t out
) {
    // N is number of sections the overall image is divided into.
    // i determines the rows of the section the kernel should operate on

    // W is width
    // H is height

    const unsigned int i = get_global_id(0);

    const int W = get_image_width(in);
    const int H = get_image_height(in);

    const int mh = H % N;              // modulo height
    const int sh = (H - mh) / N;       // segment height
    const int ly = sh * i;             // low y
    const int hy = sh * (i + 1) + ((i == (N-1)) ? mh : 0); // high y

    for (int y = ly; y < hy; ++y) {
        for (int x = 0; x < W; ++x) {
            int2 coord = {x, y};
            uint4 px_in = read_imageui(in, coord);
            float4 px_out = convert_float4(px_in) * grayscaling_factors;
            float v = px_out.x + px_out.y + px_out.z;

            write_imagef(out, coord, v);
        }
    }
}