#include "kernel_operator.h"

extern "C" __global__ __aicore__ void mode(GM_ADDR input, GM_ADDR values, GM_ADDR indices, GM_ADDR workspace, GM_ADDR tiling) {
    GET_TILING_DATA(tiling_data, tiling);
    // TODO: user kernel impl
}