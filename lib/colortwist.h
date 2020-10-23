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

    /// Values that represent the status of an operation.
    enum class StatusCode : std::uint8_t
    {
        ///< The operation completed successful.
        OK = 0,

        ///< An invalid stride was specified.
        InvalidStride = 100,

        ///< An invalid pointer (for source or destination) was specified.
        InvalidPointer = 101,


        ///< An invalid implementation type was specified.
        UnknownImplementation = 102,

        ///< An unspecified error occurred.
        UnspecifiedError = 103
    };

    StatusCode colorTwistRGB48(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);
    StatusCode colorTwistRGB24(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);
    bool isAvailable(ImplementationType type);
}
