#include "colortwist.h"
#include <limits>
#include "colortwist_config.h"
#include "colortwist_avx.h"
#include "colortwist_ipp.h"
#include "colortwist_neon.h"

using namespace std;

template <typename t>
static t floatToInteger(float f)
{
    if (f > numeric_limits<t>::max())
        return numeric_limits<t>::max();

    if (f < numeric_limits<t>::min())
        return numeric_limits<t>::min();

    return (t)(f + 0.5f);
}

template <typename t>
bool colorTwistRGB_Generic(const void *pSrc, std::uint32_t width, std::uint32_t height, std::int32_t strideSrc, void *pDst, int strideDst, const float *twistMatrix)
{
    for (size_t y = 0; y < height; ++y)
    {
        const t *ps = (const t*)(((const uint8_t*)pSrc) + y * strideSrc);
        t *pd = (t*)(((uint8_t *)pDst) + y * strideDst);
        for (size_t x = 0; x < width; ++x)
        {
            float r = *ps++;
            float g = *ps++;
            float b = *ps++;

            float rDst = r * twistMatrix[0] + g * twistMatrix[1] + b * twistMatrix[2] + twistMatrix[3];
            float gDst = r * twistMatrix[4] + g * twistMatrix[5] + b * twistMatrix[6] + twistMatrix[7];
            float bDst = r * twistMatrix[8] + g * twistMatrix[9] + b * twistMatrix[10] + twistMatrix[11];

            *pd++ = floatToInteger<t>(rDst);
            *pd++ = floatToInteger<t>(gDst);
            *pd++ = floatToInteger<t>(bDst);
        }
    }

    return true;
}

bool colorTwistRGB24_C(const void *pSrc, std::uint32_t width, std::uint32_t height, std::int32_t strideSrc, void *pDst, int strideDst, const float *twistMatrix)
{
    return colorTwistRGB_Generic<uint8_t>(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
}

bool colorTwistRGB48_C(const void *pSrc, std::uint32_t width, std::uint32_t height, std::int32_t strideSrc, void *pDst, int strideDst, const float *twistMatrix)
{
    return colorTwistRGB_Generic<uint16_t>(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
}

bool colortwist::colorTwistRGB48(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix)
{
    switch (type)
    {
    case ImplementationType::PlainC:
        return colorTwistRGB48_C(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#if COLORTWISTLIB_HASAVX
    case ImplementationType::X64_AVX:
        return colorTwistRGB48_AVX(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#endif
#if COLORTWISTLIB_HASIPP
    case ImplementationType::IPP:
        return colorTwistRGB48_IPP(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#endif
#if COLORTWISTLIB_HASNEON
    case ImplementationType::ARM_NEON:
        return colorTwistRGB48_NEON(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#endif
    }

    return false;
}

bool colortwist::isAvailable(ImplementationType type)
{
    switch (type)
    {
    case ImplementationType::PlainC:
        return true;
#if COLORTWISTLIB_HASAVX
    case ImplementationType::X64_AVX:
        return true;
#endif
#if COLORTWISTLIB_HASIPP
    case ImplementationType::IPP:
        return true;
#endif
#if COLORTWISTLIB_HASNEON
    case ImplementationType::ARM_NEON:
        return true;
#endif
    }

    return false;

}
