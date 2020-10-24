#include "colortwist.h"
#include "colortwist_config.h"
#include "colortwist_avx.h"
#include "colortwist_c.h"
#include "colortwist_ipp.h"
#include "colortwist_neon.h"

using namespace std;
using namespace  colortwist;

StatusCode colortwist::colorTwistRGB48(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix)
{
    switch (type)
    {
    case ImplementationType::PlainC:
        return colorTwistRGB48_C(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#if COLORTWISTLIB_HASAVX
    case ImplementationType::X64_AVX:
        return colorTwistRGB48_AVX(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    case ImplementationType::X64_AVX2:
        return colorTwistRGB48_AVX2(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    case ImplementationType::X64_AVX3:
        return colorTwistRGB48_AVX3(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#endif
#if COLORTWISTLIB_HASIPP
    case ImplementationType::IPP:
        return colorTwistRGB48_IPP(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#endif
#if COLORTWISTLIB_HASNEON
    case ImplementationType::ARM_NEON:
        return colorTwistRGB48_NEON(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    case ImplementationType::ARM_NEON2:
        return colorTwistRGB48_NEON2(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#endif
    }

    return StatusCode::UnknownImplementation;
}

StatusCode colortwist::colorTwistRGB24(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix)
{
    switch (type)
    {
    case ImplementationType::PlainC:
        return colorTwistRGB24_C(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#if COLORTWISTLIB_HASAVX
    case ImplementationType::X64_AVX3:
        return colorTwistRGB24_AVX3(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#endif
#if COLORTWISTLIB_HASIPP
    case ImplementationType::IPP:
        return colorTwistRGB24_IPP(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#endif
#if COLORTWISTLIB_HASNEON
    //case ImplementationType::ARM_NEON:
    //    return colorTwistRGB48_NEON(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    case ImplementationType::ARM_NEON2:
        return colorTwistRGB24_NEON2(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#endif
    }

    return StatusCode::UnknownImplementation;
}

bool colortwist::isAvailable(ImplementationType type)
{
    switch (type)
    {
    case ImplementationType::PlainC:
        return true;
#if COLORTWISTLIB_HASAVX
    case ImplementationType::X64_AVX:
    case ImplementationType::X64_AVX2:
    case ImplementationType::X64_AVX3:
        return true;
#endif
#if COLORTWISTLIB_HASIPP
    case ImplementationType::IPP:
        return true;
#endif
#if COLORTWISTLIB_HASNEON
    case ImplementationType::ARM_NEON:
    case ImplementationType::ARM_NEON2:
        return true;
#endif
    }

    return false;
}
