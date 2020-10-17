#pragma once
#include <cstdint>

bool colorTwistRGB48_NEON(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix);
