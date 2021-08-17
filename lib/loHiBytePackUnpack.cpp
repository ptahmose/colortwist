#include "loHiBytePackUnpack.h"
#include "colortwist_config.h"

#if COLORTWISTLIB_HASSSSE3
#include <tmmintrin.h>  // SSSE3
#endif

LoHiBytePackUnpack::StatusCode LoHiBytePackUnpack::LoHiBytePack_C(const void* ptrSrc, void* ptrDst, size_t length)
{
    if (length == 0)
    {
        return StatusCode::OK;
    }

    if (length % 2 != 0)
    {
        return  StatusCode::InvalidArgument;
    }

    const uint8_t* pSrc = static_cast<const uint8_t*>(ptrSrc);
    uint16_t* pDst = static_cast<uint16_t*>(ptrDst);

    const size_t halfLength = length / 2;
    for (size_t i = 0; i < halfLength; ++i)
    {
        const uint16_t v = *pSrc | (static_cast<uint16_t>(*(pSrc + halfLength)) << 8);
        *pDst++ = v;
        ++pSrc;
    }

    return StatusCode::OK;
}

#if COLORTWISTLIB_HASSSSE3
LoHiBytePackUnpack::StatusCode LoHiBytePackUnpack::LoHiBytePack_SSE(const void* ptrSrc, void* ptrDst, size_t length)
{
    if (length == 0)
    {
        return StatusCode::OK;
    }

    if (length % 2 != 0)
    {
        return StatusCode::InvalidArgument;
    }

    const uint8_t* pSrc = static_cast<const uint8_t*>(ptrSrc);
    __m128i* pDst = static_cast<__m128i*>(ptrDst);

    const size_t halfLength = length / 2;
    const size_t halfLengthOver8 = halfLength / 8;
    for (size_t i = 0; i < halfLengthOver8; ++i)
    {
        const __m128i a = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc));
        const __m128i b = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc + halfLength));
        const __m128i packed = _mm_unpacklo_epi8(a, b);
        _mm_storeu_si128(pDst++, packed);
        pSrc += 8;
    }

    const size_t remainder = halfLength % 8;
    if (remainder > 0)
    {
        uint16_t* pDstWord = reinterpret_cast<uint16_t*>(pDst);
        for (size_t i = 0; i < remainder; ++i)
        {
            const uint16_t v = *pSrc | (static_cast<uint16_t>(*(pSrc + halfLength)) << 8);
            *pDstWord++ = v;
            ++pSrc;
        }
    }

    return StatusCode::OK;
}
#else
LoHiBytePackUnpack::StatusCode LoHiBytePackUnpack::LoHiBytePack_SSE(const void* ptrSrc, void* ptrDst, size_t length)
{
    return StatusCode::InvalidISA;
}
#endif