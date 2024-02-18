// NOTE: only works with square matrices
__kernel void madd(
    const int N,
    __global float *A,
    __global float *B,
    __global float *C
) {
    int i = get_global_id(0);
    int j = get_global_id(1);
    
    C[i*N + j] = A[i*N + j] + B[i*N + j];
}
