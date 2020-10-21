#pragma once
#include <cstdint>
#include "colortwist.h"

colortwist::StatusCode colorTwistRGB48_AVX(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);
colortwist::StatusCode colorTwistRGB48_AVX2(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);
colortwist::StatusCode colorTwistRGB48_AVX3(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);