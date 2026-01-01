#include "kernel_operator.h"

// Helper struct for Compute logic to handle SFINAE/differentiation between types
template<typename T, bool IsOneByte>
struct FmaxCompute {
    __aicore__ static inline void Exec(AscendC::LocalTensor<T>& zLocal, AscendC::LocalTensor<T>& xLocal, AscendC::LocalTensor<T>& yLocal, uint32_t len, uint32_t tileLength, AscendC::TBuf<AscendC::TPosition::VECCALC>& calcBuf) {
        AscendC::SetVectorMask<T, AscendC::MaskMode::COUNTER>(len);
        AscendC::Max(zLocal, xLocal, yLocal, len);
        AscendC::ResetMask();
    }
};

// Specialization for 1-byte types (int8, uint8, bool)
template<typename T>
struct FmaxCompute<T, true> {
    __aicore__ static inline void Exec(AscendC::LocalTensor<T>& zLocal, AscendC::LocalTensor<T>& xLocal, AscendC::LocalTensor<T>& yLocal, uint32_t len, uint32_t tileLength, AscendC::TBuf<AscendC::TPosition::VECCALC>& calcBuf) {
        AscendC::LocalTensor<half> tmpX = calcBuf.Get<half>();
        AscendC::LocalTensor<half> tmpY = tmpX[tileLength];
        AscendC::LocalTensor<half> tmpZ = tmpY[tileLength];
        
        AscendC::Cast(tmpX, xLocal, AscendC::RoundMode::CAST_NONE, len);
        AscendC::Cast(tmpY, yLocal, AscendC::RoundMode::CAST_NONE, len);
        
        AscendC::SetVectorMask<half, AscendC::MaskMode::COUNTER>(len);
        AscendC::Max(tmpZ, tmpX, tmpY, len);
        AscendC::ResetMask();
        
        AscendC::Cast(zLocal, tmpZ, AscendC::RoundMode::CAST_NONE, len);
    }
};

template<typename DTYPE>
class KernelFmax {
public:
    __aicore__ inline KernelFmax() {}
    __aicore__ inline void Init(GM_ADDR input, GM_ADDR other, GM_ADDR out, uint32_t coreDataLength, uint32_t offset, uint32_t tileLength, uint32_t ALIGN_NUM)
    {
        // Init Tiling Args
        this->coreDataLength = coreDataLength;
        this->tileLength = tileLength;
        this->ALIGN_NUM = ALIGN_NUM;

        // Calculate tileNum
        this->tileNum = (coreDataLength + tileLength - 1) / tileLength;

        xGm.SetGlobalBuffer((__gm__ DTYPE*)input + offset, coreDataLength);
        yGm.SetGlobalBuffer((__gm__ DTYPE*)other + offset, coreDataLength);
        zGm.SetGlobalBuffer((__gm__ DTYPE*)out + offset, coreDataLength);

        pipe.InitBuffer(inQueueX, 1, tileLength * sizeof(DTYPE));
        pipe.InitBuffer(inQueueY, 1, tileLength * sizeof(DTYPE));
        pipe.InitBuffer(outQueue, 1, tileLength * sizeof(DTYPE));
        
        if (sizeof(DTYPE) == 1) {
            // Alloc buffer for casting to half: x_half, y_half, z_half
            pipe.InitBuffer(calcBuf, tileLength * sizeof(half) * 3);
        }
    }
    
    __aicore__ inline void Process()
    {
        for (int32_t i = 0; i < tileNum; i++) {
            uint32_t curTileLength = tileLength;
            if (i == tileNum - 1) {
                curTileLength = coreDataLength - (tileNum - 1) * tileLength;
            }
            
            CopyIn(i, curTileLength);
            Compute(i, curTileLength);
            CopyOut(i, curTileLength);
        }
    }

private:
    __aicore__ inline void CopyIn(int32_t progress, uint32_t len)
    {
        AscendC::LocalTensor<DTYPE> xLocal = inQueueX.AllocTensor<DTYPE>();
        AscendC::LocalTensor<DTYPE> yLocal = inQueueY.AllocTensor<DTYPE>();
        
        if (len % ALIGN_NUM != 0) {
            uint32_t len_bytes = len * sizeof(DTYPE);
            uint32_t remain = len_bytes % 32;
            uint32_t padding_bytes = (32 - remain) % 32;
            uint8_t rightPadding = padding_bytes / sizeof(DTYPE);

            AscendC::DataCopyExtParams copyParams{1, len_bytes, 0, 0, 0};
            AscendC::DataCopyPadExtParams<DTYPE> padParams{true, 0, rightPadding, 0};
            
            AscendC::DataCopyPad(xLocal, xGm[progress * tileLength], copyParams, padParams);
            AscendC::DataCopyPad(yLocal, yGm[progress * tileLength], copyParams, padParams);
        } else {
            AscendC::DataCopy(xLocal, xGm[progress * tileLength], len);
            AscendC::DataCopy(yLocal, yGm[progress * tileLength], len);
        }
        
        inQueueX.EnQue(xLocal);
        inQueueY.EnQue(yLocal);
    }
    
    __aicore__ inline void Compute(int32_t progress, uint32_t len)
    {
        AscendC::LocalTensor<DTYPE> xLocal = inQueueX.DeQue<DTYPE>();
        AscendC::LocalTensor<DTYPE> yLocal = inQueueY.DeQue<DTYPE>();
        AscendC::LocalTensor<DTYPE> zLocal = outQueue.AllocTensor<DTYPE>();
        
        // Use helper to dispatch based on type size
        FmaxCompute<DTYPE, (sizeof(DTYPE) == 1)>::Exec(zLocal, xLocal, yLocal, len, tileLength, calcBuf);
        
        outQueue.EnQue(zLocal);
        inQueueX.FreeTensor(xLocal);
        inQueueY.FreeTensor(yLocal);
    }
    
    __aicore__ inline void CopyOut(int32_t progress, uint32_t len)
    {
        AscendC::LocalTensor<DTYPE> zLocal = outQueue.DeQue<DTYPE>();
        
        if (len % ALIGN_NUM != 0) {
            uint32_t len_bytes = len * sizeof(DTYPE);
            AscendC::DataCopyExtParams copyParams{1, len_bytes, 0, 0, 0};
            AscendC::DataCopyPad(zGm[progress * tileLength], zLocal, copyParams);
        }
        else {
            AscendC::DataCopy(zGm[progress * tileLength], zLocal, len);
        }
        
        outQueue.FreeTensor(zLocal);
    }

private:
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX, inQueueY;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueue;
    AscendC::TBuf<AscendC::TPosition::VECCALC> calcBuf;
    
    AscendC::GlobalTensor<DTYPE> xGm;
    AscendC::GlobalTensor<DTYPE> yGm;
    AscendC::GlobalTensor<DTYPE> zGm;
    
    uint32_t coreDataLength;
    uint32_t tileNum;
    uint32_t tileLength;
    uint32_t ALIGN_NUM;
};

extern "C" __global__ __aicore__ void fmax(GM_ADDR input, GM_ADDR other, GM_ADDR out, GM_ADDR workspace, GM_ADDR tiling) {
    GET_TILING_DATA(tiling_data, tiling);
    
    uint32_t totalLength = tiling_data.totalLength;
    uint32_t tileLength = tiling_data.tileLength;
    uint32_t ALIGN_NUM = tiling_data.ALIGN_NUM;
    
    uint32_t block_idx = AscendC::GetBlockIdx();
    uint32_t block_num = AscendC::GetBlockNum();
    
    uint32_t coreDataLength = totalLength / block_num;
    uint32_t remain = totalLength % block_num;
    uint32_t offset = block_idx * coreDataLength + (block_idx < remain ? block_idx : remain);
    if (block_idx < remain) coreDataLength++;
    
    if (TILING_KEY_IS(1)) {
        KernelFmax<int8_t> op;
        op.Init(input, other, out, coreDataLength, offset, tileLength, ALIGN_NUM);
        op.Process();
    } else if (TILING_KEY_IS(2)) {
        KernelFmax<float> op;
        op.Init(input, other, out, coreDataLength, offset, tileLength, ALIGN_NUM);
        op.Process();
    } else if (TILING_KEY_IS(3)) {
        KernelFmax<half> op;
        op.Init(input, other, out, coreDataLength, offset, tileLength, ALIGN_NUM);
        op.Process();
    } else if (TILING_KEY_IS(4)) {
        // KernelFmax<bfloat16_t> op;
        // op.Init(input, other, out, coreDataLength, offset, tileLength, ALIGN_NUM);
        // op.Process();
    } else if (TILING_KEY_IS(5)) {
        // KernelFmax<int64_t> op;
        // op.Init(input, other, out, coreDataLength, offset, tileLength, ALIGN_NUM);
        // op.Process();
    } else if (TILING_KEY_IS(6)) {
        KernelFmax<int32_t> op;
        op.Init(input, other, out, coreDataLength, offset, tileLength, ALIGN_NUM);
        op.Process();
    } else if (TILING_KEY_IS(7)) {
        KernelFmax<int16_t> op;
        op.Init(input, other, out, coreDataLength, offset, tileLength, ALIGN_NUM);
        op.Process();
    } else if (TILING_KEY_IS(8)) {
        KernelFmax<int8_t> op;
        op.Init(input, other, out, coreDataLength, offset, tileLength, ALIGN_NUM);
        op.Process();
    } else if (TILING_KEY_IS(9)) {
        KernelFmax<uint8_t> op;
        op.Init(input, other, out, coreDataLength, offset, tileLength, ALIGN_NUM);
        op.Process();
    }
}
