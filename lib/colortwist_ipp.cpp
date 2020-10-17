#include "colortwist.h"
#include "colortwist_config.h"

#if COLORTWISTLIB_HASIPP
#include <ipp.h>

bool colorTwistRGB48_IPP(const void *pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void *pDst, std::int32_t strideDst, const float *twistMatrix)
{
    ippiColorTwist32f_16u_C3R((const Ipp16u*)pSrc, strideSrc,(Ipp16u*)pDst, strideDst, IppiSize{ (int)width,(int)height }, (const Ipp32f(*)[4]) twistMatrix);

    return true;
}

#endif 