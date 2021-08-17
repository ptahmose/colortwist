#include "loHiBytePackUnpack.h"
#include "colortwist_config.h"

#if COLORTWISTLIB_HASAVX

#include <immintrin.h>  // ->  AVX, AVX2, FMA

#if defined(_MSC_VER) && !defined(__AVX2__)
#error "Must be compiled with option /arch:AVX2"
#endif

LoHiBytePackUnpack::StatusCode LoHiBytePackUnpack::LoHiBytePack_AVX(const void* ptrSrc, void* ptrDst, size_t length)
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
    __m256i* pDst = static_cast<__m256i*>(ptrDst);

    const size_t halfLength = length / 2;
    const size_t halfLengthOver16 = halfLength / 16;

    for (size_t i = 0; i < halfLengthOver16; ++i)
    {
        const __m256i a = _mm256_permute4x64_epi64(_mm256_castsi128_si256(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(pSrc))), 0x50);
        const __m256i b = _mm256_permute4x64_epi64(_mm256_castsi128_si256(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(pSrc + halfLength))), 0x50);
        const __m256i packed = _mm256_unpacklo_epi8(a, b);
        _mm256_storeu_si256(pDst++, packed);
        pSrc += 16;
    }

    const size_t remainder = halfLength % 16;
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

    _mm256_zeroall();
    return StatusCode::OK;
}

#else
LoHiBytePackUnpack::StatusCode LoHiBytePackUnpack::LoHiBytePack_AVX(const void* pSrc, void* ptrDst, size_t length)
{
    return StatusCode::InvalidISA;
}
#endif
