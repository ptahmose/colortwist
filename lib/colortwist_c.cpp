#include "colortwist_c.h"
#include "utils.h"
#include <limits>
#include <cmath>

using namespace std;
using namespace  colortwist;

template <typename t>
inline static t floatToInteger(float f)
{
    if (f > numeric_limits<t>::max())
        return numeric_limits<t>::max();

    if (f < numeric_limits<t>::min())
        return numeric_limits<t>::min();

    return static_cast<t>(f + 0.5f);
}

template <typename t>
StatusCode colorTwistRGB_Generic(const void* pSrc, std::uint32_t width, std::uint32_t height, std::int32_t strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    for (size_t y = 0; y < height; ++y)
    {
        const t* ps = reinterpret_cast<const t*>(static_cast<const uint8_t*>(pSrc) + y * strideSrc);
        t* pd = reinterpret_cast<t*>(static_cast<uint8_t*>(pDst) + y * strideDst);
        for (size_t x = 0; x < width; ++x)
        {
            const float r = *ps++;
            const float g = *ps++;
            const float b = *ps++;

            const float rDst = r * twistMatrix[0] + g * twistMatrix[1] + b * twistMatrix[2] + twistMatrix[3];
            const float gDst = r * twistMatrix[4] + g * twistMatrix[5] + b * twistMatrix[6] + twistMatrix[7];
            const float bDst = r * twistMatrix[8] + g * twistMatrix[9] + b * twistMatrix[10] + twistMatrix[11];

            *pd++ = floatToInteger<t>(rDst);
            *pd++ = floatToInteger<t>(gDst);
            *pd++ = floatToInteger<t>(bDst);
        }
    }

    return StatusCode::OK;
}

void colorTwistRGB24_C_Line(const void* pSrc, uint32_t width, void* pDst, const float* twistMatrix)
{
    const uint8_t* ps = static_cast<const uint8_t*>(pSrc);
    uint8_t* pd = static_cast<uint8_t*>(pDst);
    for (size_t x = 0; x < width; ++x)
    {
        const float r = *ps++;
        const float g = *ps++;
        const float b = *ps++;

        const float rDst = r * twistMatrix[0] + g * twistMatrix[1] + b * twistMatrix[2] + twistMatrix[3];
        const float gDst = r * twistMatrix[4] + g * twistMatrix[5] + b * twistMatrix[6] + twistMatrix[7];
        const float bDst = r * twistMatrix[8] + g * twistMatrix[9] + b * twistMatrix[10] + twistMatrix[11];

        *pd++ = floatToInteger<uint8_t>(rDst);
        *pd++ = floatToInteger<uint8_t>(gDst);
        *pd++ = floatToInteger<uint8_t>(bDst);
    }
}

StatusCode colorTwistRGB24_C(const void* pSrc, std::uint32_t width, std::uint32_t height, std::int32_t strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    const colortwist::StatusCode status = checkArgumentsRgb24(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (status != colortwist::StatusCode::OK)
    {
        return status;
    }

    return colorTwistRGB_Generic<uint8_t>(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
}

StatusCode colorTwistRGB48_C(const void* pSrc, std::uint32_t width, std::uint32_t height, std::int32_t strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    const colortwist::StatusCode status = checkArgumentsRgb48(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (status != colortwist::StatusCode::OK)
    {
        return status;
    }

    return colorTwistRGB_Generic<uint16_t>(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
}