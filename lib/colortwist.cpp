#include "colortwist.h"
#include "colortwist_config.h"
#include "colortwist_avx.h"
#include "colortwist_sse.h"
#include "colortwist_c.h"
#include "colortwist_ipp.h"
#include "colortwist_neon.h"
#include "utils.h"

using namespace std;
using namespace  colortwist;

#if COLORTWISTLIB_HASAVX_INTRINSICS || COLORTWISTLIB_HASSSE42_INTRINSICS
static int hasAvx = -1;
static int hasSse41 = -1;

bool CanAvx()
{
    if (hasAvx >= 0)
    {
        return hasAvx > 0;
    }

    hasAvx = CheckWhetherCpuSupportsAVX2() ? 1 : 0;
    return hasAvx;
}

bool CanSse41()
{
    if (hasSse41 >= 0)
    {
        return hasSse41 > 0;
    }

    hasSse41 = CheckWhetherCpuSupportsSSE41() ? 1 : 0;
    return hasSse41;
}
#endif

#if COLORTWISTLIB_HASNEON
static int hasNeon = -1;

bool CanNeon()
{
    if (hasNeon >= 0)
    {
        return hasNeon > 0;
    }

    hasNeon = CheckWhetherCpuSupportsNeon() ? 1 : 0;
    return hasNeon;
}
#endif

StatusCode colortwist::colorTwistRGB48(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix)
{
    switch (type)
    {
        case ImplementationType::PlainC:
            return colorTwistRGB48_C(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
        case ImplementationType::X64_AVX:
#if COLORTWISTLIB_HASAVX_INTRINSICS_INTRINSICS
            return CanAvx() ? colorTwistRGB48_AVX(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
            return StatusCode::InvalidISA;
#endif
        case ImplementationType::X64_AVX2:
#if COLORTWISTLIB_HASAVX_INTRINSICS_INTRINSICS
            return CanAvx() ? colorTwistRGB48_AVX2(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
            return StatusCode::InvalidISA;
#endif
        case ImplementationType::X64_AVX3:
#if COLORTWISTLIB_HASAVX_INTRINSICS_INTRINSICS
            return CanAvx() ? colorTwistRGB48_AVX3(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
            return StatusCode::InvalidISA;
#endif
        case ImplementationType::IPP:
#if COLORTWISTLIB_HASIPP
            return colorTwistRGB48_IPP(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#else
            return StatusCode::NotAvailable;
#endif
        case ImplementationType::ARM_NEON:
#if COLORTWISTLIB_HASNEON
            return CanNeon() ? colorTwistRGB48_NEON(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
            return StatusCode::InvalidISA;
#endif
        case ImplementationType::ARM_NEON2:
#if COLORTWISTLIB_HASNEON
            return CanNeon() ? colorTwistRGB48_NEON2(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;;
#else
            return StatusCode::InvalidISA;
#endif
        case ImplementationType::X86_SSE:
#if COLORTWISTLIB_HASAVX_INTRINSICS
            return CanSse41() ? colorTwistRGB48_SSE(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
            return StatusCode::InvalidISA;
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
        case ImplementationType::X64_AVX3:
#if COLORTWISTLIB_HASAVX_INTRINSICS
            return CanAvx() ? colorTwistRGB24_AVX3(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
            return StatusCode::InvalidISA;
#endif
        case ImplementationType::IPP:
#if COLORTWISTLIB_HASIPP
            return colorTwistRGB24_IPP(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
#else
            return StatusCode::NotAvailable;
#endif
        case ImplementationType::ARM_NEON:
            return StatusCode::NotAvailable;
        case ImplementationType::ARM_NEON2:
#if COLORTWISTLIB_HASNEON
            return CanNeon() ? colorTwistRGB24_NEON2(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
            return StatusCode::InvalidISA;
#endif
        case ImplementationType::X86_SSE:
#if COLORTWISTLIB_HASAVX_INTRINSICS
            return CanSse41() ? colorTwistRGB24_SSE(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
            return StatusCode::InvalidISA;
#endif
    }

    return StatusCode::UnknownImplementation;
}

bool colortwist::isOperationalRgb24(ImplementationType type)
{
    switch (type)
    {
        case ImplementationType::PlainC:
            return true;
        case ImplementationType::X64_AVX3:
#if COLORTWISTLIB_HASAVX_INTRINSICS
            return CanAvx();
#else
            return false;
#endif
        case ImplementationType::IPP:
#if COLORTWISTLIB_HASIPP
            return true;
#else
            return false;
#endif
        case ImplementationType::ARM_NEON:
            return false;
        case ImplementationType::ARM_NEON2:
#if COLORTWISTLIB_HASNEON
            return CanNeon();
#else
            return false;
#endif
        case ImplementationType::X86_SSE:
#if COLORTWISTLIB_HASAVX_INTRINSICS
            return CanSse41();
#else
            return false;
#endif
    }

    return false;
}

bool colortwist::isOperationalRgb48(ImplementationType type)
{
    switch (type)
    {
        case ImplementationType::PlainC:
            return true;
        case ImplementationType::X64_AVX:
#if COLORTWISTLIB_HASAVX_INTRINSICS
            return CanAvx();
#else
            return false;
#endif
        case ImplementationType::X64_AVX2:
#if COLORTWISTLIB_HASAVX_INTRINSICS
            return CanAvx();
#else
            return false;
#endif
        case ImplementationType::X64_AVX3:
#if COLORTWISTLIB_HASAVX_INTRINSICS
            return CanAvx();
#else
            return false;
#endif
        case ImplementationType::IPP:
#if COLORTWISTLIB_HASIPP
            return true;
#else
            return false;
#endif
        case ImplementationType::ARM_NEON:
#if COLORTWISTLIB_HASNEON
            return CanNeon();
#else
            return false;
#endif
        case ImplementationType::ARM_NEON2:
#if COLORTWISTLIB_HASNEON
            return CanNeon();
#else
            return false;
#endif
        case ImplementationType::X86_SSE:
#if COLORTWISTLIB_HASAVX_INTRINSICS
            return CanSse41();
#else
            return false;
#endif
    }

    return false;
}