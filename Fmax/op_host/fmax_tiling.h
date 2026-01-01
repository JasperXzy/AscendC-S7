#ifndef FMAX_TILING_H
#define FMAX_TILING_H
#include "register/tilingdata_base.h"

namespace optiling {
BEGIN_TILING_DATA_DEF(FmaxTilingData)
  TILING_DATA_FIELD_DEF(uint32_t, totalLength);
  TILING_DATA_FIELD_DEF(uint32_t, tileNum);
  TILING_DATA_FIELD_DEF(uint32_t, tileLength);
  TILING_DATA_FIELD_DEF(uint32_t, lasttileLength);
  TILING_DATA_FIELD_DEF(uint32_t, ALIGN_NUM);
END_TILING_DATA_DEF;

REGISTER_TILING_DATA_CLASS(Fmax, FmaxTilingData)
}
#endif  // FMAX_TILING_H
