#pragma once

#include <cstdint>

namespace colortwist
{
    enum class ImplementationType : std::uint8_t
    {
        PlainC,
        IPP,
        X64_AVX,
        //ARM_NEON,
        ARM_NEON2,
        X86_SSE,
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

    /// Perform the color-twist operation on the specified RGB48-bitmap. If the operation is successful, the 
    /// destination bitmap will contain the result of the operation. The source bitmap is not modified.
    ///
    /// \param          type        An enum choosing the implementation to be used.
    /// \param          pSrc        Pointer to the source bitmap.
    /// \param          width       The width of source (and destination) bitmap in pixels.
    /// \param          height      The height of source (and destination) bitmap in pixels.
    /// \param          strideSrc   The stride of the source bitmap in bytes.
    /// \param [out]    pDst        Pointer to the destination bitmap in bytes.
    /// \param          strideDst   The stride of the source bitmap in bytes.
    /// \param          twistMatrix The color twist matrix.
    ///
    /// \returns    A status code indicating success or failure of the operation.
    StatusCode colorTwistRGB48(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);

    /// Perform the color-twist operation on the specified RGB24-bitmap. If the operation is successful, the 
    /// destination bitmap will contain the result of the operation. The source bitmap is not modified.
    ///
    /// \param          type        An enum choosing the implementation to be used.
    /// \param          pSrc        Pointer to the source bitmap.
    /// \param          width       The width of source (and destination) bitmap in pixels.
    /// \param          height      The height of source (and destination) bitmap in pixels.
    /// \param          strideSrc   The stride of the source bitmap in bytes.
    /// \param [out]    pDst        Pointer to the destination bitmap in bytes.
    /// \param          strideDst   The stride of the source bitmap in bytes.
    /// \param          twistMatrix The color twist matrix.
    ///
    /// \returns    A status code indicating success or failure of the operation.
    StatusCode colorTwistRGB24(ImplementationType type, const void* pSrc, std::uint32_t width, std::uint32_t height, int strideSrc, void* pDst, std::int32_t strideDst, const float* twistMatrix);

    /// Query if the specified implementation type' is available for RGB24-bitmaps and operational.
    /// \param type The implementation type.
    /// \returns True if operational, false if not.
    bool isOperationalRgb24(ImplementationType type);

    /// Query if the specified implementation type' is available for RGB48-bitmaps and operational.
    /// \param type The implementation type.
    /// \returns True if operational, false if not.
    bool isOperationalRgb48(ImplementationType type);

    /// Gets implementation type as an informal string. The returned string is a pointer to a statically allocated string
    /// (and must not be freed).
    /// \param  type    The implementation type.
    /// \returns    The implementation type as informal string.
    const char* GetImplementationTypeAsInformalString(ImplementationType type);
}
