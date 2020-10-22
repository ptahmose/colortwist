#pragma once
#include <cmath>
#include <cinttypes>
#include "colortwist.h"

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