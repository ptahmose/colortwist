#pragma once
#include <cstdint>
#include "colortwist.h"

colortwist::StatusCode colorTwistRGB48_IPP(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);
colortwist::StatusCode colorTwistRGB24_IPP(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);
