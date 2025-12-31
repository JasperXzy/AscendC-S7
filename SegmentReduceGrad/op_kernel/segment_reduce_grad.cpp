#include "kernel_operator.h"

extern "C" __global__ __aicore__ void segment_reduce_grad(GM_ADDR grad, GM_ADDR output, GM_ADDR data, GM_ADDR lengths, GM_ADDR offsets, GM_ADDR initial, GM_ADDR grad_output, GM_ADDR workspace, GM_ADDR tiling) {
    GET_TILING_DATA(tiling_data, tiling);
    // TODO: user kernel impl
}