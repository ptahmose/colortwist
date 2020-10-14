#include "colortwist.h"
#include <limits>

using namespace std;

template <typename t>
static t floatToInteger(float f)
{
    if (f > numeric_limits<t>::max())
        return 255;

    if (f < numeric_limits<t>::min())
        return 0;

    return (t)(f + 0.5f);
}

template <typename t>
bool colorTwistRGB_Generic(const void *pSrc, std::uint32_t width, std::uint32_t height, std::int32_t strideSrc, void *pDst, int strideDst, const float *twistMatrix)
{
    for (size_t y = 0; y < height; ++y)
    {
        const t *ps = ((const t *)pSrc) + y * strideSrc;
        t *pd = ((t *)pDst) + y * strideDst;
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
    return colorTwistRGB_Generic<uint8_t>(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
}

