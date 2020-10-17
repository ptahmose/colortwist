#include "colortwist.h"
#include "colortwist_config.h"
#if COLORTWISTLIB_HASAVX
#include <limits>
#include <immintrin.h>  // ->  AVX, AVX2, FMA

#if defined(_MSC_VER) && !defined(__AVX2__)
 #error "Must be compiled with option /arch:AVX2"
#endif

using namespace std;

bool colorTwistRGB48_AVX(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    __m128 t11t21t31 = _mm_setr_ps(twistMatrix[0], twistMatrix[4], twistMatrix[8], 0);
    __m128 t12t22t32 = _mm_setr_ps(twistMatrix[1], twistMatrix[5], twistMatrix[9], 0);
    __m128 t13t23t33 = _mm_setr_ps(twistMatrix[2], twistMatrix[6], twistMatrix[10], 0);
    __m128 t14t24t34 = _mm_setr_ps(twistMatrix[3], twistMatrix[7], twistMatrix[11], 0);

    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = (const uint16_t*)(((const uint8_t*)pSrc) + y * strideSrc);
        uint16_t* d = (uint16_t*)(((uint8_t*)pDst) + y * strideDst);
        for (size_t x = 0; x < width; ++x)
        {
            __m128 r_src = _mm_set1_ps(p[0]);
            __m128 g_src = _mm_set1_ps(p[1]);
            __m128 b_src = _mm_set1_ps(p[2]);

            __m128 result = _mm_fmadd_ps(r_src, t11t21t31, t14t24t34);
            result = _mm_fmadd_ps(g_src, t12t22t32, result);
            result = _mm_fmadd_ps(b_src, t13t23t33, result);

            __m128i resultInteger = _mm_cvtps_epi32(result);
            __m128i resultShort = _mm_packus_epi32(resultInteger, resultInteger);

            uint64_t _3words = _mm_cvtsi128_si64(resultShort);
            *((uint32_t*)d) = (uint32_t)_3words;
            *(d+2) = (uint16_t)(_3words >> 32); 

            p += 3;
            d += 3;
        }
    }

    return true;
}
#endif