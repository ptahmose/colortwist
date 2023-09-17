#include "colortwist.h"
#include "colortwist_config.h"
#include "colortwist_sse.h"

#include <immintrin.h>


#if COLORTWISTLIB_HASAVX
#include "utils.h"
#include <limits>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>  // ->  AVX, AVX2, FMA

//#if defined(_MSC_VER) && !defined(__AVX2__)
//#error "Must be compiled with option /arch:AVX2"
//#endif

using namespace std;
using namespace colortwist;

__m128i load_unaligned_dword(const void* ptr) {
    return _mm_castps_si128(_mm_load_ss((const float*)ptr));
}

void store_unaligned_dword(void* ptr, __m128i value) {
    _mm_store_ss((float*)ptr, _mm_castsi128_ps(value));
}

colortwist::StatusCode colorTwistRGB24_SSE(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    __m128 matrix_row1 = _mm_setr_ps(twistMatrix[0], twistMatrix[1], twistMatrix[2], twistMatrix[3]);
    __m128 matrix_row2 = _mm_setr_ps(twistMatrix[4], twistMatrix[5], twistMatrix[6], twistMatrix[7]);
    __m128 matrix_row3 = _mm_setr_ps(twistMatrix[8], twistMatrix[9], twistMatrix[10], twistMatrix[11]);

    for (size_t y = 0; y < height; ++y)
    {
        const uint8_t* ps = static_cast<const uint8_t*>(pSrc) + y * strideSrc;
        uint8_t* pd = static_cast<uint8_t*>(pDst) + y * strideDst;
        for (uint32_t x = 0; x < width/4; ++x)
        {
            __m128i a = _mm_loadu_si64(ps);
            //__m128i b = /*_mm_loadu_si32*/load_unaligned_dword(ps + 8);
            __m128i b = _mm_castps_si128(_mm_load_ss((const float*)(ps + 8)));

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

           // __m128i result1_ui8 = _mm_packus_epi16(result1_ui16, result1_ui16);
           // __m128i result2_ui8 = _mm_packus_epi16(result2_ui16, result2_ui16);
            __m128i result3_ui8 = _mm_packus_epi16(result3_ui16, result3_ui16);

            //__m128i result12_ui8 = _mm_unpackhi_epi32(result1_ui8, result2_ui8);
            __m128i result12_ui8 = _mm_packus_epi16(result12_ui16, result12_ui16);

            _mm_storeu_si64(reinterpret_cast<__m128i*>(pd), result12_ui8);
    //        /*_mm_storeu_si32*/store_unaligned_dword(pd + 8, result3_ui8);
            _mm_store_ss((float*)(pd + 8), _mm_castsi128_ps(result3_ui8));

            pd += 3 * 4;
            ps += 3 * 4;
        }
    }

    return colortwist::StatusCode::OK;
}

colortwist::StatusCode _colorTwistRGB24_SSE(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    __m128 matrix_row1 = _mm_setr_ps(twistMatrix[0], twistMatrix[1], twistMatrix[2], twistMatrix[3]);
    __m128 matrix_row2 = _mm_setr_ps(twistMatrix[4], twistMatrix[5], twistMatrix[6], twistMatrix[7]);
    __m128 matrix_row3 = _mm_setr_ps(twistMatrix[8], twistMatrix[9], twistMatrix[10], twistMatrix[11]);
    for (size_t y = 0; y < height; ++y)
    {
        const uint8_t* ps = static_cast<const uint8_t*>(pSrc) + y * strideSrc;
        uint8_t* pd = static_cast<uint8_t*>(pDst) + y * strideDst;

        for (uint32_t x = 0; x < width; ++x)
        {
            int s = ps[0] | (ps[1] << 8) | (ps[2] << 16) | 0x01000000;
            __m128i a = _mm_set1_epi32(s);

            //__m128i a = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(ps));
            __m128i b = _mm_unpacklo_epi8(a, _mm_setzero_si128());
            __m128i c = _mm_unpacklo_epi16(b, _mm_setzero_si128());

            // Convert packed 32-bit integers to packed 32-bit floats
            __m128 vecFloat = _mm_cvtepi32_ps(c);

            __m128 sum1 = _mm_dp_ps(vecFloat, matrix_row1, 0xF1);
            __m128 sum2 = _mm_dp_ps(vecFloat, matrix_row2, 0xF2);
            __m128 sum3 = _mm_dp_ps(vecFloat, matrix_row3, 0xF4);
            __m128 result = _mm_or_ps(_mm_or_ps(sum1, sum2), sum3);

            __m128i result_ui32 = _mm_cvtps_epi32(result);

            // Pack the 32-bit integers down to 16-bit integers
            // In this step, values are saturated to fit into int16_t. This means values below INT16_MIN become INT16_MIN and values above INT16_MAX become INT16_MAX.
            __m128i vecInt16 = _mm_packs_epi32(result_ui32, result_ui32); // The same input is used twice as a trick to only use the low 4 values

            // Pack the 16-bit integers down to 8-bit integers
            // Similarly, values are saturated to fit into int8_t.
            __m128i vecInt8 = _mm_packus_epi16(vecInt16, vecInt16); // The same trick is used here

            uint32_t r = (uint32_t)_mm_cvtsi128_si32(vecInt8);

            pd[0] = (uint8_t)r;
            pd[1] = (uint8_t)(r >> 8);
            pd[2] = (uint8_t)(r >> 16);

            /*pd[0] = _mm_extract_epi8(vecInt8, 0);
            pd[1] = _mm_extract_epi8(vecInt8, 1);
            pd[2] = _mm_extract_epi8(vecInt8, 2);*/

            pd += 3;
            ps += 3;
        }
    }
    return colortwist::StatusCode::OK;
}


#endif
