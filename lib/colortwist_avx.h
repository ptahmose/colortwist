#pragma once
#include <cstdint>
#include "colortwist.h"

colortwist::StatusCode colorTwistRGB48_AVX(const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);
colortwist::StatusCode colorTwistRGB24_AVX(const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);