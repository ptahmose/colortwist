#pragma once

#include <cstdint>

namespace colortwist
{
    enum class ImplementationType : std::uint8_t
    {
        PlainC,
        IPP,
        X64_AVX,
        X64_AVX2,
        X64_AVX3,
        ARM_NEON,
        ARM_NEON2
    };

    bool colorTwistRGB48(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);
    bool isAvailable(ImplementationType type);
}

//bool colorTwistRGB24_C(const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);
//bool colorTwistRGB48_C(const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);
//
//#if COLORTWISTLIB_HASIPP
//bool colorTwistRGB48_IPP(const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);
//#endif
//
//#if COLORTWISTLIB_HASAVX
//bool colorTwistRGB48_AVX(const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);
//#endif