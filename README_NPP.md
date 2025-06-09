# NVIDIA Performance Primitives (NPP) Acceleration

This fork of TA‑Lib adds optional GPU acceleration via [NVIDIA Performance Primitives](https://developer.nvidia.com/npp). When enabled, many vector math functions use NPP to process data on the GPU, while preserving the original API.

## Enabling NPP support

1. Install the CUDA Toolkit which provides the NPP libraries.
2. Configure the build with `USE_NPP=ON`:

```bash
cmake -S . -B build -DUSE_NPP=ON
cmake --build build -j$(nproc)
```

If the CUDA Toolkit cannot be found, the build system automatically disables NPP and falls back to the CPU implementation.

## Accelerated functions

When compiled with `USE_NPP`, the following core routines call their GPU implementations automatically:

- `TA_ADD`, `TA_SUB`, `TA_MULT`, `TA_DIV`
- `TA_SUM`, `TA_SMA`, `TA_SQRT`, `TA_AVGPRICE`
- `TA_MIN`, `TA_MAX`, `TA_CORREL`, `TA_LINEARREG_SLOPE`
- Trigonometric and exponential transforms such as `TA_SIN`, `TA_COS`, `TA_TAN`, `TA_EXP`, `TA_LN`, `TA_LOG10`, `TA_ACOS`, `TA_ASIN`, `TA_ATAN`, `TA_CEIL`, `TA_FLOOR`, `TA_SINH`, `TA_COSH`, `TA_TANH`

The same C APIs are used. For example, calling `TA_SMA` with NPP enabled executes the GPU‑accelerated version; no source changes are needed in user code.

For advanced usage, NPP-specific entry points like `TA_ADD_NPP` are declared in `ta_func.h`. These functions perform no parameter checking and assume valid indices.

## Example

```c
#include <ta_func.h>

/* build with -DUSE_NPP=ON */
TA_RetCode ret = TA_SMA(0, length-1, prices, 14, &beg, &nb, output);
```

This call uses `nppsSum` internally when NPP is available and falls back to the original CPU implementation otherwise.

