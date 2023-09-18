#pragma once
#include <cmath>
#include <cinttypes>
#include "colortwist.h"
#include "colortwist_config.h"

inline colortwist::StatusCode checkArgumentsRgb48(const void* pSrc, std::uint32_t width, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    if (pSrc == nullptr || pDst == nullptr || twistMatrix == nullptr)
    {
        return colortwist::StatusCode::InvalidPointer;
    }

    if (width * 3 * 2 < (std::uint32_t)abs(strideSrc) || width * 3 * 2 < (std::uint32_t)abs(strideDst))
    {
        return colortwist::StatusCode::InvalidStride;
    }

    return colortwist::StatusCode::OK;
}

inline colortwist::StatusCode checkArgumentsRgb24(const void* pSrc, std::uint32_t width, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    if (pSrc == nullptr || pDst == nullptr || twistMatrix == nullptr)
    {
        return colortwist::StatusCode::InvalidPointer;
    }

    if (width * 3 < (std::uint32_t)abs(strideSrc) || width * 3 < (std::uint32_t)abs(strideDst))
    {
        return colortwist::StatusCode::InvalidStride;
    }

    return colortwist::StatusCode::OK;
}

#if COLORTWISTLIB_HASAVX_INTRINSICS || COLORTWISTLIB_HASSSE42_INTRINSICS
bool CheckWhetherCpuSupportsAVX2();
bool CheckWhetherCpuSupportsSSE41();
#endif

#if COLORTWISTLIB_HASNEON
bool CheckWhetherCpuSupportsNeon();
#endif