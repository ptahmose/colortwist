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
            *(d + 2) = (uint16_t)(_3words >> 32);

            p += 3;
            d += 3;
        }
    }

    return true;
}

bool colorTwistRGB48_AVX2(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    /*__m128 t11t21t31 =  _mm256_broadcastsi128_si256(_mm_setr_ps(twistMatrix[0], twistMatrix[4], twistMatrix[8], 0));
    __m128 t12t22t32 = _mm_setr_ps(twistMatrix[1], twistMatrix[5], twistMatrix[9], 0);
    __m128 t13t23t33 = _mm_setr_ps(twistMatrix[2], twistMatrix[6], twistMatrix[10], 0);
    __m128 t14t24t34 = _mm_setr_ps(twistMatrix[3], twistMatrix[7], twistMatrix[11], 0);*/

    __m256 t11t21t31 = _mm256_setr_ps(twistMatrix[0], twistMatrix[4], twistMatrix[8], 0, twistMatrix[0], twistMatrix[4], twistMatrix[8], 0);
    __m256 t12t22t32 = _mm256_setr_ps(twistMatrix[1], twistMatrix[5], twistMatrix[9], 0, twistMatrix[1], twistMatrix[5], twistMatrix[9], 0);
    __m256 t13t23t33 = _mm256_setr_ps(twistMatrix[2], twistMatrix[6], twistMatrix[10], 0, twistMatrix[2], twistMatrix[6], twistMatrix[10], 0);
    __m256 t14t24t34 = _mm256_setr_ps(twistMatrix[3], twistMatrix[7], twistMatrix[11], 0, twistMatrix[3], twistMatrix[7], twistMatrix[11], 0);

    __m256i shufflConst1 = _mm256_setr_epi32(0, 0, 0, 0, 3, 3, 3, 3);
    __m256i shufflConst2 = _mm256_setr_epi32(1, 1, 1, 1, 4, 4, 4, 4);
    __m256i shufflConst3 = _mm256_setr_epi32(2, 2, 2, 2, 5, 5, 5, 5);

    __m128i shuffleConst4 = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 0, 0, 0, 0);
    //__m128i moveMask = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0);

    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = (const uint16_t*)(((const uint8_t*)pSrc) + y * strideSrc);
        uint16_t* d = (uint16_t*)(((uint8_t*)pDst) + y * strideDst);
        for (size_t x = 0; x < width / 4; ++x)
        {
            __m128i src1 = _mm_loadu_si128((const __m128i*)p);
            __m256i src1Int32 = _mm256_cvtepu16_epi32(src1);
            __m256 src1Float = _mm256_cvtepi32_ps(src1Int32);

            int rg = _mm_extract_epi32(src1, 3);
            __m128i src2 = _mm_loadu_si64((const __m128i*)(p + 8));
            __m128i src2_ = _mm_bslli_si128(src2, 4);
            __m128i src2__ = _mm_insert_epi32(src2_, rg, 0);
            __m256i src2Int32 = _mm256_cvtepu16_epi32(src2__);
            __m256 src2Float = _mm256_cvtepi32_ps(src2Int32);

            __m256 r1r2 = _mm256_permutevar8x32_ps(src1Float, shufflConst1);
            __m256 g1g2 = _mm256_permutevar8x32_ps(src1Float, shufflConst2);
            __m256 b1b2 = _mm256_permutevar8x32_ps(src1Float, shufflConst3);

            __m256 result = _mm256_fmadd_ps(r1r2, t11t21t31, t14t24t34);
            result = _mm256_fmadd_ps(g1g2, t12t22t32, result);
            result = _mm256_fmadd_ps(b1b2, t13t23t33, result);

            __m256i resultInteger = _mm256_cvtps_epi32(result);
            __m256i resultUShorts = _mm256_packus_epi32(resultInteger, _mm256_permute2f128_si256(resultInteger, resultInteger, 0x55));
            __m128i resultUShorts2_1 = _mm_shuffle_epi8(_mm256_castsi256_si128(resultUShorts), shuffleConst4);
            //_mm_maskmoveu_si128(resultUShorts2, moveMask, (char*)d);



            r1r2 = _mm256_permutevar8x32_ps(src2Float, shufflConst1);
            g1g2 = _mm256_permutevar8x32_ps(src2Float, shufflConst2);
            b1b2 = _mm256_permutevar8x32_ps(src2Float, shufflConst3);

            result = _mm256_fmadd_ps(r1r2, t11t21t31, t14t24t34);
            result = _mm256_fmadd_ps(g1g2, t12t22t32, result);
            result = _mm256_fmadd_ps(b1b2, t13t23t33, result);

            resultInteger = _mm256_cvtps_epi32(result);
            resultUShorts = _mm256_packus_epi32(resultInteger, _mm256_permute2f128_si256(resultInteger, resultInteger, 0x55));
            __m128i resultUShorts2 = _mm_shuffle_epi8(_mm256_castsi256_si128(resultUShorts), shuffleConst4);

            int xx = _mm_extract_epi32(resultUShorts2, 0);
            __m128i r = _mm_insert_epi32(resultUShorts2_1, xx, 3);
            _mm_storeu_si128((__m128i*)d, r);
            __m128i r2 = _mm_bsrli_si128(resultUShorts2, 4);
            _mm_storel_epi64((__m128i*)(d + 8), r2);

            //_mm_maskmoveu_si128(resultUShorts2, moveMask, (char*)(d+6));


            p += 6*2;
            d += 6*2;
        }
    }

    _mm256_zeroupper();
    return true;
}

#endif