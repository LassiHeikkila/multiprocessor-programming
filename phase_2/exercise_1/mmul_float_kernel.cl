// NOTE: only works with square matrices
__kernel void mmul(
    const int N,
    __global float *A,
    __global float *B,
    __global float *C
) {
    int k;
    int i = get_global_id(0);
    int j = get_global_id(1);
    double tmp;

    if ((i < N) && (j < N)) {
        tmp = 0.0;
        for (k = 0; k < N; k++) {
            tmp += A[i*N + k] * B[k*N + j];
        }
        C[i*N + j] = (float)tmp;
    }
}
