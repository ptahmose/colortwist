#include "colortwist.h"
#include "colortwist_config.h"
#include "colortwist_sse.h"

#if COLORTWISTLIB_HAS_INTEL_INTRINSICS
#include "utils.h"
#include <limits>
#include <immintrin.h>

using namespace std;
using namespace colortwist;

/// This template is parametrized by a flag indicating whether the width is a multiple of 4. The idea is that we can create specialized
/// versions of the function where we deal with not-multiple-of-4 widths or we do not. The execution time of the function is improved
/// by not dealing with a remainder in the inner loop (if there is no remainder). The performance improvement is small but significant.
///
/// \tparam deal_with_remainder   A boolean indicating whether the function should deal with a remainder in the inner loop.
template <bool deal_with_remainder> struct ColorTwistRgb24Generic
{
    static colortwist::StatusCode do_it(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
    {
        const __m128 matrix_row1 = _mm_setr_ps(twistMatrix[0], twistMatrix[1], twistMatrix[2], twistMatrix[3]);
        const __m128 matrix_row2 = _mm_setr_ps(twistMatrix[4], twistMatrix[5], twistMatrix[6], twistMatrix[7]);
        const __m128 matrix_row3 = _mm_setr_ps(twistMatrix[8], twistMatrix[9], twistMatrix[10], twistMatrix[11]);

        const uint32_t width_over_four = width / 4;
        const uint32_t width_mod_four = width % 4;

        for (uint32_t y = 0; y < height; ++y)
        {
            const uint8_t* ps = static_cast<const uint8_t*>(pSrc) + static_cast<size_t>(y) * strideSrc;
            uint8_t* pd = static_cast<uint8_t*>(pDst) + static_cast<size_t>(y) * strideDst;
            for (uint32_t x = 0; x < width_over_four; ++x)
            {
                __m128i a = _mm_loadu_si64(ps);
#if COLORTWISTLIB_HAS_MM_LOADU_SI32_INTRINSICS
                __m128i b = _mm_loadu_si32(ps + 8);
#else
                __m128i b = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float*>(ps + 8)));
#endif

                __m128i c = _mm_unpacklo_epi8(a, _mm_setzero_si128());

                __m128i first_pixel_rgb1 = _mm_insert_epi16(c, 0x0001, 3);

                __m128i second_pixel_rgb2 = _mm_insert_epi16(_mm_bslli_si128(c, 2), 0x0001, 7);

                __m128i first_pixel_rgb1_uint32 = _mm_unpacklo_epi16(first_pixel_rgb1, _mm_setzero_si128());
                __m128i second_pixel_rgb1_uint32 = _mm_unpackhi_epi16(second_pixel_rgb2, _mm_setzero_si128());

                __m128i d = _mm_bsrli_si128(c, 12);

                __m128i e = _mm_bslli_si128(_mm_unpacklo_epi8(b, _mm_setzero_si128()), 4);
                __m128i third_pixel_rgb1 = _mm_insert_epi16(_mm_or_si128(d, e), 0x0001, 3);
                __m128i third_pixel_rgb1_uint32 = _mm_unpacklo_epi16(third_pixel_rgb1, _mm_setzero_si128());

                __m128i fourth_pixel_rgb1 = _mm_insert_epi16(_mm_bslli_si128(e, 2), 0x0001, 7);
                __m128i fourth_pixel_rgb1_uint32 = _mm_unpackhi_epi16(fourth_pixel_rgb1, _mm_setzero_si128());

                __m128 first_pixel_rgb1_float = _mm_cvtepi32_ps(first_pixel_rgb1_uint32);
                __m128 second_pixel_rgb1_float = _mm_cvtepi32_ps(second_pixel_rgb1_uint32);
                __m128 third_pixel_rgb1_float = _mm_cvtepi32_ps(third_pixel_rgb1_uint32);
                __m128 fourth_pixel_rgb1_float = _mm_cvtepi32_ps(fourth_pixel_rgb1_uint32);

                __m128 sum1 = _mm_dp_ps(first_pixel_rgb1_float, matrix_row1, 0xF1);
                __m128 sum2 = _mm_dp_ps(first_pixel_rgb1_float, matrix_row2, 0xF2);
                __m128 sum3 = _mm_dp_ps(first_pixel_rgb1_float, matrix_row3, 0xF4);
                __m128 sum4 = _mm_dp_ps(second_pixel_rgb1_float, matrix_row1, 0xF8);
                __m128 result1 = _mm_or_ps(_mm_or_ps(sum1, sum2), _mm_or_ps(sum3, sum4));

                sum1 = _mm_dp_ps(second_pixel_rgb1_float, matrix_row2, 0xF1);
                sum2 = _mm_dp_ps(second_pixel_rgb1_float, matrix_row3, 0xF2);
                sum3 = _mm_dp_ps(third_pixel_rgb1_float, matrix_row1, 0xF4);
                sum4 = _mm_dp_ps(third_pixel_rgb1_float, matrix_row2, 0xF8);
                __m128 result2 = _mm_or_ps(_mm_or_ps(sum1, sum2), _mm_or_ps(sum3, sum4));

                sum1 = _mm_dp_ps(third_pixel_rgb1_float, matrix_row3, 0xF1);
                sum2 = _mm_dp_ps(fourth_pixel_rgb1_float, matrix_row1, 0xF2);
                sum3 = _mm_dp_ps(fourth_pixel_rgb1_float, matrix_row2, 0xF4);
                sum4 = _mm_dp_ps(fourth_pixel_rgb1_float, matrix_row3, 0xF8);
                __m128 result3 = _mm_or_ps(_mm_or_ps(sum1, sum2), _mm_or_ps(sum3, sum4));

                __m128i result1_ui32 = _mm_cvtps_epi32(result1);
                __m128i result2_ui32 = _mm_cvtps_epi32(result2);
                __m128i result3_ui32 = _mm_cvtps_epi32(result3);

                __m128i result1_ui16 = _mm_packs_epi32(result1_ui32, result1_ui32);
                __m128i result2_ui16 = _mm_packs_epi32(result2_ui32, result2_ui32);
                __m128i result3_ui16 = _mm_packs_epi32(result3_ui32, result3_ui32);

                __m128i result12_ui16 = _mm_unpacklo_epi64(result1_ui16, result2_ui16);

                __m128i result3_ui8 = _mm_packus_epi16(result3_ui16, result3_ui16);

                __m128i result12_ui8 = _mm_packus_epi16(result12_ui16, result12_ui16);

                _mm_storeu_si64(reinterpret_cast<__m128i*>(pd), result12_ui8);
#if COLORTWISTLIB_HAS_MM_STOREU_SI32_INTRINSICS
                _mm_storeu_si32(pd + 8, result3_ui8);
#else
                _mm_store_ss(reinterpret_cast<float*>(pd + 8), _mm_castsi128_ps(result3_ui8));
#endif

                pd += 12;
                ps += 12;
            }

            if (deal_with_remainder)
            {
                for (uint32_t x = 0; x < width_mod_four; ++x)
                {
                    int s = ps[0] | (ps[1] << 8) | (ps[2] << 16) | 0x01000000;
                    __m128i a = _mm_set1_epi32(s);
                    __m128i b = _mm_unpacklo_epi8(a, _mm_setzero_si128());
                    __m128i c = _mm_unpacklo_epi16(b, _mm_setzero_si128());
                    __m128 vecFloat = _mm_cvtepi32_ps(c);
                    __m128 sum1 = _mm_dp_ps(vecFloat, matrix_row1, 0xF1);
                    __m128 sum2 = _mm_dp_ps(vecFloat, matrix_row2, 0xF2);
                    __m128 sum3 = _mm_dp_ps(vecFloat, matrix_row3, 0xF4);
                    __m128 result = _mm_or_ps(_mm_or_ps(sum1, sum2), sum3);
                    __m128i result_ui32 = _mm_cvtps_epi32(result);
                    __m128i vecInt16 = _mm_packs_epi32(result_ui32, result_ui32); // The same input is used twice as a trick to only use the low 4 values
                    __m128i vecInt8 = _mm_packus_epi16(vecInt16, vecInt16); // The same trick is used here
                    uint32_t r = static_cast<uint32_t>(_mm_cvtsi128_si32(vecInt8));
                    pd[0] = static_cast<uint8_t>(r);
                    pd[1] = static_cast<uint8_t>(r >> 8);
                    pd[2] = static_cast<uint8_t>(r >> 16);

                    ps += 3;
                    pd += 3;
                }
            }
        }

        return colortwist::StatusCode::OK;
    }
};

colortwist::StatusCode colorTwistRGB24_SSE(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    const colortwist::StatusCode status = checkArgumentsRgb24(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (status != colortwist::StatusCode::OK)
    {
        return status;
    }

    if ((width % 4) != 0)
    {
        return ColorTwistRgb24Generic<true>::do_it(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    }
    else
    {
        return ColorTwistRgb24Generic<false>::do_it(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    }
}

template <bool deal_with_remainder> struct ColorTwistRgb48Generic
{
    static colortwist::StatusCode do_it(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
    {
        const __m128 matrix_row1 = _mm_setr_ps(twistMatrix[0], twistMatrix[1], twistMatrix[2], twistMatrix[3]);
        const __m128 matrix_row2 = _mm_setr_ps(twistMatrix[4], twistMatrix[5], twistMatrix[6], twistMatrix[7]);
        const __m128 matrix_row3 = _mm_setr_ps(twistMatrix[8], twistMatrix[9], twistMatrix[10], twistMatrix[11]);

        const uint32_t width_over_two = width / 2;
        const uint32_t width_mod_two = width % 2;

        for (uint32_t y = 0; y < height; ++y)
        {
            const uint8_t* ps = static_cast<const uint8_t*>(pSrc) + static_cast<size_t>(y) * strideSrc;
            uint8_t* pd = static_cast<uint8_t*>(pDst) + static_cast<size_t>(y) * strideDst;
            for (uint32_t x = 0; x < width_over_two; ++x)
            {
                __m128i first_rgbr_ushort16 = _mm_loadu_si64(ps);
                __m128i first_rgb1_ushort16 = _mm_insert_epi16(first_rgbr_ushort16, 0x0001, 3);

                __m128i c = _mm_castps_si128(_mm_load_ss((const float*)(ps + 8)));
                __m128i second_rgb1_ushort16 = _mm_unpacklo_epi32(_mm_shuffle_epi32(first_rgbr_ushort16, 0x55), c);
                second_rgb1_ushort16 = _mm_insert_epi16(second_rgb1_ushort16, 0x0001, 0);

                __m128i first_rgb1_uint32 = _mm_unpacklo_epi16(first_rgb1_ushort16, _mm_setzero_si128());
                __m128  first_rgb1_ = _mm_cvtepi32_ps(first_rgb1_uint32);

                __m128i second_rgb1_uint32 = _mm_unpacklo_epi16(second_rgb1_ushort16, _mm_setzero_si128());
                __m128  second_rgb1_ = _mm_cvtepi32_ps(second_rgb1_uint32);

                __m128 result_float_gb = _mm_or_ps(
                    _mm_dp_ps(second_rgb1_, _mm_shuffle_ps(matrix_row2, matrix_row2, _MM_SHUFFLE(2, 1, 0, 3)), 0xF1),
                    _mm_dp_ps(second_rgb1_, _mm_shuffle_ps(matrix_row3, matrix_row3, _MM_SHUFFLE(2, 1, 0, 3)), 0xF2));

                __m128 result_float_rgbr = _mm_or_ps(
                    _mm_or_ps(
                        _mm_or_ps(_mm_dp_ps(first_rgb1_, matrix_row1, 0xF1), _mm_dp_ps(first_rgb1_, matrix_row2, 0xF2)),
                        _mm_dp_ps(first_rgb1_, matrix_row3, 0xF4)),
                    _mm_dp_ps(second_rgb1_, _mm_shuffle_ps(matrix_row1, matrix_row1, _MM_SHUFFLE(2, 1, 0, 3)), 0xF8));

                __m128i result_uint32_rgbr = _mm_cvtps_epi32(result_float_rgbr);
                __m128i result_uint16_rgbr = _mm_packus_epi32(result_uint32_rgbr, result_uint32_rgbr); // The same input is used twice as a trick to only use the low 4 values
                _mm_storeu_si64(reinterpret_cast<__m128i*>(pd), result_uint16_rgbr);

                __m128i result_uint32_gb = _mm_cvtps_epi32(result_float_gb);
                __m128i result_uint16_gb = _mm_packus_epi32(result_uint32_gb, result_uint32_gb);
                _mm_store_ss((float*)(pd + 8), _mm_castsi128_ps(result_uint16_gb));

                ps += 12;
                pd += 12;
            }

            if (deal_with_remainder)
            {
                if (width_mod_two > 0)
                {
                    const __m128i kStoreMask = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1);

                    __m128i a = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float*>(ps)));
#if 0 && COLORTWISTLIB_HAS_MM_LOADU_SI16_INTRINSICS
                    __m128i b = _mm_loadu_si16(ps + 4); // load 2 bytes (the remainder of the SSE-register is zeroed)
#else
                    __m128i b = _mm_cvtsi32_si128((*reinterpret_cast<const uint16_t*>(ps + 4)));
#endif
                    b = _mm_insert_epi16(b, 0x0001, 3);

                    __m128i rgb1_ushort16 = _mm_unpacklo_epi32(a, b);
                    __m128i rgb1_uint32 = _mm_unpacklo_epi16(rgb1_ushort16, _mm_setzero_si128());

                    __m128 rgb1_float = _mm_cvtepi32_ps(rgb1_uint32);

                    __m128 sum1 = _mm_dp_ps(rgb1_float, matrix_row1, 0xF1);
                    __m128 sum2 = _mm_dp_ps(rgb1_float, matrix_row2, 0xF2);
                    __m128 sum3 = _mm_dp_ps(rgb1_float, matrix_row3, 0xF4);
                    __m128 result_float = _mm_or_ps(_mm_or_ps(sum1, sum2), sum3);

                    __m128i result_ui32 = _mm_cvtps_epi32(result_float);
                    __m128i result1_ui16 = _mm_packus_epi32(result_ui32, result_ui32);

                    _mm_maskmoveu_si128(result1_ui16, kStoreMask, reinterpret_cast<char*>(pd));
                }
            }
        }

        return colortwist::StatusCode::OK;
    }
};

colortwist::StatusCode colorTwistRGB48_SSE(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    const colortwist::StatusCode status = checkArgumentsRgb24(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (status != colortwist::StatusCode::OK)
    {
        return status;
    }

    if ((width % 2) != 0)
    {
        return ColorTwistRgb48Generic<true>::do_it(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    }
    else
    {
        return ColorTwistRgb48Generic<false>::do_it(pSrc, width, height, strideSrc, pDst, strideDst, twistMatrix);
    }
}

#endif
