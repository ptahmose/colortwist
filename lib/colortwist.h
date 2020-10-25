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

        ///< The selected operation requires the availability of CPU-instructions not supported by this processor.
        UnsupportedInstructionSet = 100,

        ///< The selected operation is using an ISA not supported on the processor's architecture.
        InvalidISA = 101,

        ///< An selected operation is not available.
        NotAvailable,

        ///< An invalid stride was specified.
        InvalidStride = 103,

        ///< An invalid pointer (for source or destination) was specified.
        InvalidPointer = 104,

        ///< An invalid implementation type was specified.
        UnknownImplementation = 105,

        ///< An unspecified error occurred.
        UnspecifiedError = 106
    };

    StatusCode colorTwistRGB48(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);
    StatusCode colorTwistRGB24(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);

    /// Query if the specified implementation type' is available for RGB24-bitmaps and operational.
    /// \param type The implementation type.
    /// \returns True if operational, false if not.
    bool isOperationalRgb24(ImplementationType type);

    /// Query if the specified implementation type' is available for RGB48-bitmaps and operational.
    /// \param type The implementation type.
    /// \returns True if operational, false if not.
    bool isOperationalRgb48(ImplementationType type);
}
