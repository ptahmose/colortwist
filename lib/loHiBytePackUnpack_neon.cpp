#include "loHiBytePackUnpack.h"
#include "colortwist_config.h"

#if COLORTWISTLIB_HASNEON

#include <arm_neon.h>

/*static*/LoHiBytePackUnpack::StatusCode LoHiBytePackUnpack::LoHiBytePack_Neon(const void* ptrSrc, void* ptrDst, size_t length)
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
    uint8_t* pDst = static_cast<uint8_t*>(ptrDst);

    const size_t halfLength = length / 2;
    const size_t halfLengthOver8 = halfLength / 8;
    for (size_t i = 0; i < halfLengthOver8; ++i)
    {
        const uint8x8_t a = vld1_u8(static_cast<const uint8_t*>(pSrc));
        const uint8x8_t b = vld1_u8(static_cast<const uint8_t*>(pSrc + halfLength));
        pSrc += 8;

        const auto z1 = vzip1_u8(a, b);
        const auto z2 = vzip2_u8(a, b);

        vst1_u8(pDst,z1);
        pDst += 8;
        vst1_u8( pDst, z2);
        pDst += 8;
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
/*static*/LoHiBytePackUnpack::StatusCode LoHiBytePackUnpack::LoHiBytePack_Neon(const void* pSrc, void* ptrDst, size_t length)
{
    return StatusCode::InvalidISA;
}
#endif