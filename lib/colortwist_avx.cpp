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
        const uint16_t* p = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pSrc) + y * strideSrc);
        uint16_t* d = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pDst) + y * strideDst);
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
            *reinterpret_cast<uint32_t*>(d) = static_cast<uint32_t>(_3words);
            *(d + 2) = static_cast<uint16_t>(_3words >> 32);

            p += 3;
            d += 3;
        }
    }

    return true;
}

bool colorTwistRGB48_AVX2(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    static const __m128i shuffleConst4 = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 0, 0, 0, 0);
    static const __m128i shuffleConst6 = _mm_setr_epi8(0, 1, 2, 3, 4, 5, -1, -1, 6, 7, 8, 9, 10, 11, -1, -1);

    __m128 t11t12t13t14 = _mm_loadu_ps(twistMatrix + 0);
    __m128 t21t22t23t24 = _mm_loadu_ps(twistMatrix + 4);
    __m128 t31t32t33t34 = _mm_loadu_ps(twistMatrix + 8);
    __m128 t41t42t43t44 = _mm_setzero_ps();
    _MM_TRANSPOSE4_PS(t11t12t13t14, t21t22t23t24, t31t32t33t34, t41t42t43t44);
    const __m256 t11t21t31 = _mm256_castsi256_ps(_mm256_broadcastsi128_si256(_mm_castps_si128(t11t12t13t14)));
    const __m256 t12t22t32 = _mm256_castsi256_ps(_mm256_broadcastsi128_si256(_mm_castps_si128(t21t22t23t24)));
    const __m256 t13t23t33 = _mm256_castsi256_ps(_mm256_broadcastsi128_si256(_mm_castps_si128(t31t32t33t34)));
    const __m256 t14t24t34 = _mm256_castsi256_ps(_mm256_broadcastsi128_si256(_mm_castps_si128(t41t42t43t44)));

    const size_t widthRemainder = width % 4;
    const size_t widthOver4 = width / 4;
    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pSrc) + y * strideSrc);
        uint16_t* d = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pDst) + y * strideDst);
        for (size_t x = 0; x < widthOver4; ++x)
        {
            __m128i src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));     // load 8 words -> R1 G1 B1 R2 | G2 B2 R3 G3
            __m128i src1Shuffled = _mm_shuffle_epi8(src1, shuffleConst6);            // shuffle to      R1 G1 B1 xx | R2 G2 B2 xx
            __m256i src1Int32 = _mm256_cvtepu16_epi32(src1Shuffled);                 // convert to int32
            __m256 src1Float = _mm256_cvtepi32_ps(src1Int32);                        // convert to float

            __m128i src1r3g3 = _mm_bsrli_si128(src1, 12);                            // shift so that we have R3 G3 00 00 | 00 00 00 00
            __m128i src2 = _mm_loadu_si64(p + 8);                                    // load 4 words -> B3 R4 G4 B4
            __m128i src2Shifted = _mm_bslli_si128(src2, 4);                          // shift so that we have 00 00 B3 R4 | G4 B4 xx xx
            __m128i src2Ored = _mm_or_si128(src1r3g3, src2Shifted);                  // or to get R3 G3 B3 R4 | G4 B4 xx xx
            __m128i src2Shuffled = _mm_shuffle_epi8(src2Ored, shuffleConst6);        // shuffle to R3 G3 B3 xx | R4 G4 B4 xx
            __m256i src2Int32 = _mm256_cvtepu16_epi32(src2Shuffled);                 // convert to int32
            __m256 src2Float = _mm256_cvtepi32_ps(src2Int32);                        // convert to float

            __m256 r1r2 = _mm256_shuffle_ps(src1Float, src1Float, 0);                // shuffle to get R1 R1 R1 xx | R2 R2 R2 xx
            __m256 g1g2 = _mm256_shuffle_ps(src1Float, src1Float, 0x55);             // shuffle to get G1 G1 G1 xx | G2 G2 G2 xx
            __m256 b1b2 = _mm256_shuffle_ps(src1Float, src1Float, 0xaa);             // shuffle to get B1 B1 B1 xx | B2 B2 B2 xx

            __m256 result = _mm256_fmadd_ps(r1r2, t11t21t31, t14t24t34);             // we do the matrix-multiplication  R' = t11*R + t12*G + t13*B + t14
            result = _mm256_fmadd_ps(g1g2, t12t22t32, result);                       //                                  G' = t21*R + t22*G + t23*B + t24
            result = _mm256_fmadd_ps(b1b2, t13t23t33, result);                       //                                  B' = t31*R + t32*G + t33*B + t34
                                                                                     // here, we now have R'1 G'1 B'1 xx | R'2 G'2 B'2 xx

            __m256i resultInteger = _mm256_cvtps_epi32(result);                      // convert to int32
            __m256i resultUShorts = _mm256_packus_epi32(                             // convert to words    
                resultInteger,
                _mm256_castsi128_si256(_mm256_extracti128_si256(resultInteger, 1)));

            __m128i resultUShortsShuffled = _mm_shuffle_epi8(                        // shuffle to get R'1 G'1 B'1 R'2 | G'2 B'2 xx xx (words)
                _mm256_castsi256_si128(resultUShorts),
                shuffleConst4);

            // now, the same for the third and fourth RGB-triple
            r1r2 = _mm256_shuffle_ps(src2Float, src2Float, 0);
            g1g2 = _mm256_shuffle_ps(src2Float, src2Float, 0x55);
            b1b2 = _mm256_shuffle_ps(src2Float, src2Float, 0xaa);

            result = _mm256_fmadd_ps(r1r2, t11t21t31, t14t24t34);
            result = _mm256_fmadd_ps(g1g2, t12t22t32, result);
            result = _mm256_fmadd_ps(b1b2, t13t23t33, result);

            resultInteger = _mm256_cvtps_epi32(result);
            resultUShorts = _mm256_packus_epi32(resultInteger, _mm256_castsi128_si256(_mm256_extracti128_si256(resultInteger, 1)));
            __m128i result2UShortsShuffled = _mm_shuffle_epi8(_mm256_castsi256_si128(resultUShorts), shuffleConst4);

            // we combine resultUShortsShuffled with result2UShortsShuffled to get: R'1 G'1 B'1 R'2 | G'2 B'2 R'3 G'3
            // it seems there is no "_m128i-intrinsic" corresponding to _mm_insert_ps, so we have a lot of casts here - however, those are "free" (no runtime-costs)
            __m128i r = _mm_castps_si128(_mm_insert_ps(_mm_castsi128_ps(resultUShortsShuffled), _mm_castsi128_ps(result2UShortsShuffled), 0x30));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(d), r);

            // finally, we now get B'3 R'4 G'4 B'4 and store it
            __m128i r2 = _mm_bsrli_si128(result2UShortsShuffled, 4);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(d + 8), r2);

            p += 6 * 2;
            d += 6 * 2;
        }

        // we do 4 RGB-triples per loop above, here we deal with remaining pixels in a line
        for (size_t r = 0; r < widthRemainder; ++r)
        {
            __m128 r_src = _mm_set1_ps(p[0]);
            __m128 g_src = _mm_set1_ps(p[1]);
            __m128 b_src = _mm_set1_ps(p[2]);

            __m128 result = _mm_fmadd_ps(r_src, _mm256_castps256_ps128(t11t21t31), _mm256_castps256_ps128(t14t24t34));
            result = _mm_fmadd_ps(g_src, _mm256_castps256_ps128(t12t22t32), result);
            result = _mm_fmadd_ps(b_src, _mm256_castps256_ps128(t13t23t33), result);

            __m128i resultInteger = _mm_cvtps_epi32(result);
            __m128i resultShort = _mm_packus_epi32(resultInteger, resultInteger);

            uint64_t _3words = _mm_cvtsi128_si64(resultShort);
            *reinterpret_cast<uint32_t*>(d) = static_cast<uint32_t>(_3words);
            *(d + 2) = static_cast<uint16_t>(_3words >> 32);

            p += 3;
            d += 3;
        }
    }

    _mm256_zeroupper();
    return true;
}

bool colorTwistRGB48_AVX3(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    static const __m128i shuffleConst1 = _mm_setr_epi8(0, 1, 6, 7, 12, 13, 2, 3, 8, 9, 14, 15, 4, 5, 10, 11);
    static const __m128i shuffleConst2 = _mm_setr_epi8(2, 3, 8, 9, 14, 15, 4, 5, 10, 11, 0, 01, 6, 7, 12, 13);
    static const __m128i shuffleConst3 = _mm_setr_epi8(4, 5, 10, 11, 0, 1, 6, 7, 12, 13, 2, 3, 8, 9, 14, 15);
    const float& t11 = twistMatrix[0];
    const float& t12 = twistMatrix[1];
    const float& t13 = twistMatrix[2];
    const float& t14 = twistMatrix[3];
    const float& t21 = twistMatrix[4];
    const float& t22 = twistMatrix[5];
    const float& t23 = twistMatrix[6];
    const float& t24 = twistMatrix[7];
    const float& t31 = twistMatrix[8];
    const float& t32 = twistMatrix[9];
    const float& t33 = twistMatrix[10];
    const float& t34 = twistMatrix[11];

    static const __m128i shuffleConst10 = _mm_setr_epi8(-1, -1, -1, -1, 12, 13, -1, -1, -1, -1, 14, 15, -1, -1, -1, -1);
    static const __m128i shuffleConst11 = _mm_setr_epi8(10, 11, -1, -1, -1, -1, 12, 13, -1, -1, -1, -1, 14, 15, -1, -1);
    static const __m128i shuffleConst12 = _mm_setr_epi8(-1, -1, 10, 11, -1, -1, -1, -1, 12, 13, -1, -1, -1, -1, 14, 15);

    static const __m256i shuffleConst256_1 = _mm256_setr_epi8(0, 1, -1, -1, -1, -1, 2, 3, -1, -1, -1, -1, 4, 5, -1, -1, -1, -1, 6, 7, -1, -1, -1, -1, 8, 9, -1, -1, -1, -1, 10, 11);
    static const __m256i shuffleConst256_2 = _mm256_setr_epi8(-1, -1, 0, 1, -1, -1, -1, -1, 2, 3, -1, -1, -1, -1, 4, 5, -1, -1, -1, -1, 6, 7, -1, -1, -1, -1, 8, 9, -1, -1, -1, -1);
    static const __m256i shuffleConst256_3 = _mm256_setr_epi8(-1, -1, -1, -1, 0, 1, -1, -1, -1, -1, 2, 3, -1, -1, -1, -1, 4, 5, -1, -1, -1, -1, 6, 7, -1, -1, -1, -1, 8, 9, -1, -1);

    const size_t widthRemainder = width % 8;
    const size_t widthOver8 = width / 8;
    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pSrc) + y * strideSrc);
        uint16_t* d = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pDst) + y * strideDst);
        for (size_t x = 0; x < widthOver8; ++x)
        {
            __m128i src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));     // load 8 words -> R1 G1 B1 R2 | G2 B2 R3 G3
            __m128i src1Shuffled = _mm_shuffle_epi8(src1, shuffleConst1);            // shuffle to      R1 R2 R3 G1 | G2 G3 B1 B2
            __m128i src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 8)); // load 8 words -> B3 R4 G4 B4 | R5 G5 B5 R6
            __m128i src2Shuffled = _mm_shuffle_epi8(src2, shuffleConst2);            // shuffle to      R4 R5 R6 G4 | G5 B3 B4 B5
            __m128i src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 16));// load 8 words -> G6 B6 R7 G7 | B7 R8 G8 B8
            __m128i src3Shuffled = _mm_shuffle_epi8(src3, shuffleConst3);            // shuffle to      R7 R8 G6 G7 | G8 B6 B7 B8

            __m128i redWords = _mm_blend_epi16(_mm_blend_epi16(src1Shuffled, _mm_bslli_si128(src2Shuffled, 6), 0x38), _mm_bslli_si128(src3Shuffled, 12), 0xc0);
            __m128i greenWords = _mm_blend_epi16(_mm_blend_epi16(_mm_bsrli_si128(src1Shuffled, 6), src2Shuffled, 0x38), _mm_bslli_si128(src3Shuffled, 6), 0xe0);
            __m128i blueWords = _mm_blend_epi16(_mm_blend_epi16(_mm_bsrli_si128(src1Shuffled, 12), _mm_bsrli_si128(src2Shuffled, 6), 0x3c), src3Shuffled, 0xe0);

            __m256 redFloats = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(redWords));
            __m256 greenFloats = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(greenWords));
            __m256 blueFloats = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(blueWords));

            __m256 m1 = _mm256_fmadd_ps(redFloats, _mm256_set1_ps(t11), _mm256_set1_ps(t14));
            __m256 m2 = _mm256_fmadd_ps(greenFloats, _mm256_set1_ps(t12), m1);
            __m256 resultR = _mm256_fmadd_ps(blueFloats, _mm256_set1_ps(t13), m2);

            m1 = _mm256_fmadd_ps(redFloats, _mm256_set1_ps(t21), _mm256_set1_ps(t24));
            m2 = _mm256_fmadd_ps(greenFloats, _mm256_set1_ps(t22), m1);
            __m256 resultG = _mm256_fmadd_ps(blueFloats, _mm256_set1_ps(t23), m2);

            m1 = _mm256_fmadd_ps(redFloats, _mm256_set1_ps(t31), _mm256_set1_ps(t34));
            m2 = _mm256_fmadd_ps(greenFloats, _mm256_set1_ps(t32), m1);
            __m256 resultB = _mm256_fmadd_ps(blueFloats, _mm256_set1_ps(t33), m2);

            __m256i resultInteger = _mm256_cvtps_epi32(resultR);                      // convert to int32, and then to words
            __m256i resultRUShorts = _mm256_broadcastsi128_si256( _mm256_castsi256_si128(_mm256_packus_epi32(resultInteger,_mm256_castsi128_si256(_mm256_extracti128_si256(resultInteger, 1)))));

            resultInteger = _mm256_cvtps_epi32(resultG);
            __m256i resultGUShorts = _mm256_broadcastsi128_si256(_mm256_castsi256_si128(_mm256_packus_epi32(resultInteger, _mm256_castsi128_si256(_mm256_extracti128_si256(resultInteger, 1)))));

            resultInteger = _mm256_cvtps_epi32(resultB);
            __m256i resultBUShorts = _mm256_broadcastsi128_si256(_mm256_castsi256_si128(_mm256_packus_epi32(resultInteger, _mm256_castsi128_si256(_mm256_extracti128_si256(resultInteger, 1)))));
            
            __m256i resultRShuffled1_256 = _mm256_shuffle_epi8(resultRUShorts, shuffleConst256_1);
            __m256i resultGShuffled2_256 = _mm256_shuffle_epi8(resultGUShorts, shuffleConst256_2);
            __m256i resultBShuffled3_256 = _mm256_shuffle_epi8(resultBUShorts, shuffleConst256_3);
            __m256i result256 = _mm256_or_si256(_mm256_or_si256(resultRShuffled1_256, resultGShuffled2_256), resultBShuffled3_256);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(d), result256);
            
            __m128i resultRShuffled3 = _mm_shuffle_epi8(_mm256_castsi256_si128(resultRUShorts), shuffleConst10);
            __m128i resultGShuffled3 = _mm_shuffle_epi8(_mm256_castsi256_si128(resultGUShorts), shuffleConst11);
            __m128i resultBShuffled3 = _mm_shuffle_epi8(_mm256_castsi256_si128(resultBUShorts), shuffleConst12);
            __m128i result3 = _mm_or_si128(_mm_or_si128(resultRShuffled3, resultGShuffled3), resultBShuffled3);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(d + 16), result3);

            p += 24;
            d += 24;
        }
    }

    _mm256_zeroupper();
    return true;
}

#endif