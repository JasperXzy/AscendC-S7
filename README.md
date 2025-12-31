## Fmax

Classify | Name | Type | Shape | TypeRangeAll | AttrDefaultValue | Format
-- | -- | -- | -- | -- | -- | --
INPUT | input | tensor | (..., N4,N3,N2,N) | bool,float32,float16,bfloat16,int64,int32,int16,int8,uint8 | - | ND
INPUT | other | tensor | (..., N4,N3,N2,N) | bool,float32,float16,bfloat16,int64,int32,int16,int8,uint8 | - | ND
OUTPUT | out | tensor | (..., N4,N3,N2,N) | bool,float32,float16,bfloat16,int32,int8,int64,int16,anuint8 | - | ND

### 1. 参考算子

[`torch.fmax`](https://docs.pytorch.org/docs/2.5/generated/torch.fmax.html#torch.fmax)

### 2. 算子输入取值范围

- `N ∈ [1, 10000]`
- `N2 ∈ [1, 10000]`
- `N3 ∈ [1, 1000]`
- `N4 ∈ [1, 200]`

### 3. 算子输入特征说明

- N ~ N4 均可能为非 32 的整倍数，需要考虑非对齐场景
- 数据取值范围均不会超出数据类型表达范围
- input 和 other 支持广播，需要考虑广播场景

---

## Mode

Classify | Name | Type | Shape | TypeRangeAll | AttrDefaultValue | Format
-- | -- | -- | -- | -- | -- | --
INPUT | input | tensor | [N,N2] | float32,float16,int64,int32,int16,int8,uint8 | - | ND
ATTR | dim | int32 | - | - | -1 | -
ATTR | keepdim | bool | - | - | FALSE | -
OUTPUT | values | tensor | [N] or [N,N2] | float32,float16,int64,int32,int16,int8,uint8 | - | ND
OUTPUT | indices | tensor | [N] or [N,N2] | int64,int64,int64,int64,int64,int64,int64 | - | ND

### 1. 参考算子

[`torch.mode`](https://docs.pytorch.org/docs/2.5/generated/torch.mode.html)

### 2. 算子输入取值范围

- `N ∈ [1, 10000]`
- `N2 ∈ [1, 10000]`

### 3. 算子输入特征说明

- N ~ N2 均可能为非 32 的整倍数，需要考虑非对齐场景
- 数据取值范围均不会超出数据类型表达范围

---

## PdistGrad

Classify | Name | Type | Shape | TypeRangeAll | AttrDefaultValue | Format
-- | -- | -- | -- | -- | -- | --
INPUT | grad | tensor | [N*(N-1)//2] | float32 | - | ND
INPUT | input | tensor | [N,N2] | float32 | - | ND
INPUT | pdist | tensor | [N*(N-1)//2] | float32 | - | ND
ATTR | p | float | - | - | - | -
OUTPUT | out | tensor | [N,N2] | float32 | - | ND

### 1. 参考算子

[`torch.nn.functional.pdist`](https://docs.pytorch.org/docs/2.5/generated/torch.nn.functional.pdist.html)

### 2. 算子输入取值范围

- `N ∈ [1, 256]`
- `N2 ∈ [1, 10000]`

### 3. 算子输入特征说明

- N ~ N2 均可能为非 32 的整倍数，需要考虑非对齐场景
- 数据取值范围均不会超出数据类型表达范围

---

## SegmentReduce

Classify | Name | Type | Shape | TypeRangeAll | AttrDefaultValue | Format
-- | -- | -- | -- | -- | -- | --
INPUT | data | tensor | [N,N2] | float32,float16,bfloat16 | - | ND
OPTIONAL_INPUT | lengths | tensor | [K] or [N,K] | int32,int32,int32 | - | ND
OPTIONAL_INPUT | indices | tensor | / | int32,int32,int32 | - | ND
OPTIONAL_INPUT | offsets | tensor | [K] or [N,K] | int32,int32,int32 | - | ND
OPTIONAL_INPUT | initial | tensor | [1] | float32,float16,bfloat16 | - | ND
ATTR | reduce | string | - | - | - | -
ATTR | axis | int | - | - | 0 | -
ATTR | unsafe | bool | - | - | FALSE | -
OUTPUT | out | tensor | (K, N2) if axis == 0 else (N, K) | float32,float16,bfloat16 | - | ND

### 1. 参考算子

[`torch.segment_reduce`](https://github.com/pytorch/pytorch/blob/v2.5.1/aten/src/ATen/native/SegmentReduce.cpp)

### 2. 算子输入取值范围

- `N ∈ [1, 10000]`
- `N2 ∈ [1, 10000]`
- `K ∈ [1, 20]`

### 3. 算子输入特征说明

- N ~ N2 均可能为非 32 的整倍数，需要考虑非对齐场景
- 数据取值范围均不会超出数据类型表达范围
- `offsets[-1] = data.shape[axis]`
- `sum(lengths) = data.shape[axis]`

---

## SegmentReduceGrad

Classify | Name | Type | Shape | TypeRangeAll | AttrDefaultValue | Format
-- | -- | -- | -- | -- | -- | --
INPUT | grad | tensor | - | float32,float16,bfloat16 | - | ND
INPUT | output | tensor | (K, N2) if axis == 0 else (N, K) | float32,float16,bfloat16 | - | ND
INPUT | data | tensor | [N,N2] | float32,float16,bfloat16 | - | ND
OPTIONAL_INPUT | lengths | tensor | [K] or [N,K] | int32,int32,int32 | - | ND
OPTIONAL_INPUT | offsets | tensor | [K] or [N,K] | int32,int32,int32 | - | ND
OPTIONAL_INPUT | initial | tensor | [1] | float32,float16,bfloat16 | - | ND
ATTR | reduce | string | - | - | - | -
ATTR | axis | int | - | - | 0 | -
OUTPUT | grad_output | tensor | [N,N2] | float32,float16,bfloat16 | - | ND

### 1. 参考算子

[`torch.segment_reduce`](https://github.com/pytorch/pytorch/blob/v2.5.1/aten/src/ATen/native/SegmentReduce.cpp)

### 2. 算子输入取值范围

- `N ∈ [1, 10000]`
- `N2 ∈ [1, 10000]`
- `K ∈ [1, 20]`

### 3. 算子输入特征说明

- N ~ N2 均可能为非 32 的整倍数，需要考虑非对齐场景
- 数据取值范围均不会超出数据类型表达范围
- `offsets[-1] = data.shape[axis]`
- `sum(lengths) = data.shape[axis]`