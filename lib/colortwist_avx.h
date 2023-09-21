#pragma once
#include <cstdint>
#include "colortwist.h"

colortwist::StatusCode colorTwistRGB48_AVX3(const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);
colortwist::StatusCode colorTwistRGB24_AVX3(const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);