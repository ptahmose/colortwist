#pragma once

#include <cstdint>
#include <cstddef>

class LoHiBytePackUnpack
{
public:
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
        InvalidArgument = 103,

        ///< An invalid pointer (for source or destination) was specified.
        InvalidPointer = 104,

        ///< An invalid implementation type was specified.
        UnknownImplementation = 105,

        ///< An unspecified error occurred.
        UnspecifiedError = 106
    };

    static StatusCode LoHiBytePack_C(const void* pSrc, void* ptrDst, size_t length);
    static StatusCode LoHiBytePack_SSE(const void* pSrc, void* ptrDst, size_t length);
    static StatusCode LoHiBytePack_AVX(const void* pSrc, void* ptrDst, size_t length);
    static StatusCode LoHiBytePack_Neon(const void* pSrc, void* ptrDst, size_t length);
};