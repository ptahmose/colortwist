#include "colortwist.h"
#include "colortwist_config.h"
#include "colortwist_sse.h"


#if COLORTWISTLIB_HASAVX
#include "utils.h"
#include <limits>
#include <emmintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>  // ->  AVX, AVX2, FMA

//#if defined(_MSC_VER) && !defined(__AVX2__)
//#error "Must be compiled with option /arch:AVX2"
//#endif

using namespace std;
using namespace colortwist;



colortwist::StatusCode colorTwistRGB24_SSE(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
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

            pd[0] = _mm_extract_epi8(vecInt8, 0);
            pd[1] = _mm_extract_epi8(vecInt8, 1);
            pd[2] = _mm_extract_epi8(vecInt8, 2);

            pd += 3;
            ps += 3;
        }
    }
    return colortwist::StatusCode::OK;
}


#endif
