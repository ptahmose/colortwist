#include "colortwist.h"
#include "colortwist_config.h"
#include "colortwist_avx.h"
#include "colortwist_c.h"
#include "colortwist_ipp.h"
#include "colortwist_neon.h"
#include "utils.h"

using namespace std;
using namespace  colortwist;

#if COLORTWISTLIB_HASAVX
static int hasAvx = -1;

bool CanAvx()
{
    if (hasAvx >= 0)
    {
        return hasAvx > 0;
    }

    hasAvx = CheckWhetherCpuSupportsAVX2() ? 1 : 0;
    return hasAvx;
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
#if COLORTWISTLIB_HASAVX
        return CanAvx() ? colorTwistRGB48_AVX(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
        return StatusCode::InvalidISA;
#endif
    case ImplementationType::X64_AVX2:
#if COLORTWISTLIB_HASAVX
        return CanAvx() ? colorTwistRGB48_AVX2(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix) : StatusCode::UnsupportedInstructionSet;
#else
        return StatusCode::InvalidISA;
#endif
    case ImplementationType::X64_AVX3:
#if COLORTWISTLIB_HASAVX
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
#if COLORTWISTLIB_HASAVX
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
