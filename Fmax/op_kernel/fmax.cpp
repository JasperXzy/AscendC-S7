#include "kernel_operator.h"

template<typename DTYPE>
class KernelFmax {
public:
    __aicore__ inline KernelFmax() {}
    __aicore__ inline void Init(GM_ADDR input, GM_ADDR other, GM_ADDR out, uint32_t totalLength, uint32_t tileNum, uint32_t tileLength, uint32_t lasttileLength, uint32_t ALIGN_NUM)
    {
        // Init Tiling Args
        this->totalLength = totalLength;
        this->tileNum = tileNum;
        this->tileLength = tileLength;
        this->lasttileLength = lasttileLength;
        this->ALIGN_NUM = ALIGN_NUM;

        // Init GM Pointers
        uint32_t coreDataLength = totalLength / AscendC::GetBlockNum();
        uint32_t offset = AscendC::GetBlockIdx() * coreDataLength;
        
        xGm.SetGlobalBuffer((__gm__ DTYPE*)input + offset, coreDataLength);
        yGm.SetGlobalBuffer((__gm__ DTYPE*)other + offset, coreDataLength);
        zGm.SetGlobalBuffer((__gm__ DTYPE*)out + offset, coreDataLength);

        // Init Buffer
        pipe.InitBuffer(inQueueX, 1, tileLength * sizeof(DTYPE));
        pipe.InitBuffer(inQueueY, 1, tileLength * sizeof(DTYPE));
        pipe.InitBuffer(outQueue, 1, tileLength * sizeof(DTYPE));
    }
    
    __aicore__ inline void Process()
    {
        for (int32_t i = 0; i < tileNum; i++) {
            uint32_t curTileLength = (i == tileNum - 1) ? lasttileLength : tileLength;
            
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
        
        AscendC::DataCopy(xLocal, xGm[progress * tileLength], len);
        AscendC::DataCopy(yLocal, yGm[progress * tileLength], len);
        
        inQueueX.EnQue(xLocal);
        inQueueY.EnQue(yLocal);
    }
    
    __aicore__ inline void Compute(int32_t progress, uint32_t len)
    {
        AscendC::LocalTensor<DTYPE> xLocal = inQueueX.DeQue<DTYPE>();
        AscendC::LocalTensor<DTYPE> yLocal = inQueueY.DeQue<DTYPE>();
        AscendC::LocalTensor<DTYPE> zLocal = outQueue.AllocTensor<DTYPE>();
        
        AscendC::Max(zLocal, xLocal, yLocal, len);
        
        outQueue.EnQue(zLocal);
        inQueueX.FreeTensor(xLocal);
        inQueueY.FreeTensor(yLocal);
    }
    
    __aicore__ inline void CopyOut(int32_t progress, uint32_t len)
    {
        AscendC::LocalTensor<DTYPE> zLocal = outQueue.DeQue<DTYPE>();
        AscendC::DataCopy(zGm[progress * tileLength], zLocal, len);
        outQueue.FreeTensor(zLocal);
    }

private:
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX, inQueueY;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueue;
    
    AscendC::GlobalTensor<DTYPE> xGm;
    AscendC::GlobalTensor<DTYPE> yGm;
    AscendC::GlobalTensor<DTYPE> zGm;
    
    uint32_t totalLength;
    uint32_t tileNum;
    uint32_t tileLength;
    uint32_t lasttileLength;
    uint32_t ALIGN_NUM;
};

extern "C" __global__ __aicore__ void fmax(GM_ADDR input, GM_ADDR other, GM_ADDR out, GM_ADDR workspace, GM_ADDR tiling) {
    GET_TILING_DATA(tiling_data, tiling);
    KernelFmax<half> op;
    op.Init(input, other, out, tiling_data.totalLength, tiling_data.tileNum, tiling_data.tileLength, tiling_data.lasttileLength, tiling_data.ALIGN_NUM);
    op.Process();
}
