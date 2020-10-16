#pragma once

#include "colortwist_config.h"

#include <cstdint>

bool colorTwistRGB24_C(const void *pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void *pDst, std::int32_t strideDst, const float *twistMatrix);
bool colorTwistRGB48_C(const void *pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void *pDst, std::int32_t strideDst, const float *twistMatrix);

#if COLORTWISTLIB_HASIPP
 bool colorTwistRGB48_IPP(const void *pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void *pDst, std::int32_t strideDst, const float *twistMatrix);
#endif

#if COLORTWISTLIB_HASAVX
 bool colorTwistRGB48_AVX(const void *pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void *pDst, std::int32_t strideDst, const float *twistMatrix);
#endif