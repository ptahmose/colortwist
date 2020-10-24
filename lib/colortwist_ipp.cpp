#include "colortwist.h"
#include "colortwist_config.h"

#if COLORTWISTLIB_HASIPP
#include "utils.h"
#include <ipp.h>

using namespace colortwist;

StatusCode colorTwistRGB48_IPP(const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix)
{
    StatusCode rc = checkArgumentsRgb48(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (rc != colortwist::StatusCode::OK)
    {
        return rc;
    }

    IppStatus status = ippiColorTwist32f_16u_C3R((const Ipp16u*)pSrc, strideSrc, (Ipp16u*)pDst, strideDst, IppiSize{ (int)width,(int)height }, (const Ipp32f(*)[4]) twistMatrix);

    return (status == ippStsNoErr || status == ippStsNoOperation) ? StatusCode::OK : StatusCode::UnspecifiedError;
}

colortwist::StatusCode colorTwistRGB24_IPP(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    StatusCode rc = checkArgumentsRgb24(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (rc != colortwist::StatusCode::OK)
    {
        return rc;
    }

    IppStatus status = ippiColorTwist32f_8u_C3R((const Ipp8u*)pSrc, strideSrc, (Ipp8u*)pDst, strideDst, IppiSize{ (int)width,(int)height }, (const Ipp32f(*)[4]) twistMatrix);

    return (status == ippStsNoErr || status == ippStsNoOperation) ? StatusCode::OK : StatusCode::UnspecifiedError;
}

#endif 