
#include "fmax_tiling.h"
#include "register/op_def_registry.h"


namespace optiling {
const uint32_t BLOCK_DIM = 8;
const uint32_t UB_SIZE = 192 * 1024; // 192KB for data

static ge::graphStatus TilingFunc(gert::TilingContext* context)
{
  FmaxTilingData tiling;
  const gert::StorageShape* x1_shape = context->GetInputShape(0);
  uint32_t totalLength = 1;
  for (int i = 0; i < x1_shape->GetStorageShape().GetDimNum(); i++) {
    totalLength *= x1_shape->GetStorageShape().GetDim(i);
  }
  
  auto dt = context->GetInputTensor(0)->GetDataType();
  uint32_t sizeof_datatype = ge::GetSizeByDataType(dt);
  uint32_t ALIGN_NUM = 32 / sizeof_datatype;

  // Calculate max elements per tile based on UB size
  uint32_t max_tile_elements = (UB_SIZE / 3) / sizeof_datatype;

  context->SetBlockDim(BLOCK_DIM);

  uint32_t elements_per_core = (totalLength + BLOCK_DIM - 1) / BLOCK_DIM;
  
  uint32_t tileLength = 0;
  if (elements_per_core > max_tile_elements) {
      tileLength = max_tile_elements;
      // Must align tileLength
      tileLength = (tileLength / ALIGN_NUM) * ALIGN_NUM;
  } else {
      tileLength = elements_per_core;
      // Align tileLength upwards if needed, but last tile handles remainder
      tileLength = (tileLength + ALIGN_NUM - 1) / ALIGN_NUM * ALIGN_NUM;
  }

  // Avoid tileLength being 0
  if (tileLength == 0) tileLength = ALIGN_NUM;
  
  uint32_t tileNum = (elements_per_core + tileLength - 1) / tileLength;
  uint32_t lasttileLength = elements_per_core % tileLength;
  if (lasttileLength == 0) lasttileLength = tileLength;

  tiling.set_totalLength(totalLength);
  tiling.set_tileNum(tileNum);
  tiling.set_tileLength(tileLength);
  tiling.set_lasttileLength(lasttileLength);
  tiling.set_ALIGN_NUM(ALIGN_NUM);

  tiling.SaveToBuffer(context->GetRawTilingData()->GetData(), context->GetRawTilingData()->GetCapacity());
  context->GetRawTilingData()->SetDataSize(tiling.GetDataSize());

  return ge::GRAPH_SUCCESS;
}
}


namespace ge {
static ge::graphStatus InferShape(gert::InferShapeContext* context)
{
    const gert::Shape* x1_shape = context->GetInputShape(0);
    gert::Shape* y_shape = context->GetOutputShape(0);
    *y_shape = *x1_shape;
    return GRAPH_SUCCESS;
}
static ge::graphStatus InferDataType(gert::InferDataTypeContext *context)
{
const auto inputDataType = context->GetInputDataType(0);
context->SetOutputDataType(0, inputDataType);
return ge::GRAPH_SUCCESS;
}
}


namespace ops {
class Fmax : public OpDef {
public:
    explicit Fmax(const char* name) : OpDef(name)
    {
        this->Input("input")
            .ParamType(REQUIRED)
            .DataType({ge::DT_BOOL, ge::DT_FLOAT, ge::DT_FLOAT16, ge::DT_BF16, ge::DT_INT64, ge::DT_INT32, ge::DT_INT16, ge::DT_INT8, ge::DT_UINT8})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND})
            .UnknownShapeFormat({ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND});
        this->Input("other")
            .ParamType(REQUIRED)
            .DataType({ge::DT_BOOL, ge::DT_FLOAT, ge::DT_FLOAT16, ge::DT_BF16, ge::DT_INT64, ge::DT_INT32, ge::DT_INT16, ge::DT_INT8, ge::DT_UINT8})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND})
            .UnknownShapeFormat({ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND});
        this->Output("out")
            .ParamType(REQUIRED)
            .DataType({ge::DT_BOOL, ge::DT_FLOAT, ge::DT_FLOAT16, ge::DT_BF16, ge::DT_INT64, ge::DT_INT32, ge::DT_INT16, ge::DT_INT8, ge::DT_UINT8})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND})
            .UnknownShapeFormat({ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND});

        this->SetInferShape(ge::InferShape).SetInferDataType(ge::InferDataType);

        this->AICore()
            .SetTiling(optiling::TilingFunc);
        this->AICore().AddConfig("ascend910b");

    }
};

OP_ADD(Fmax);
}
