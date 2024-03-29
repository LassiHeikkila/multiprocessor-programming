// NOTE: only works with square matrices
__kernel void mmul(
    const int N,
    __global int *A,
    __global int *B,
    __global int *C
) {
    int k;
    int i = get_global_id(0);
    int j = get_global_id(1);
    int tmp;

    if ((i < N) && (j < N)) {
        tmp = 0;
        for (k = 0; k < N; k++) {
            tmp += A[i*N + k] * B[k*N + j];
        }
        C[i*N + j] = tmp;
    }
}
