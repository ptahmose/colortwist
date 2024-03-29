#include "colortwist.h"
#include "colortwist_config.h"
#if COLORTWISTLIB_HAS_INTEL_INTRINSICS
#include "utils.h"
#include <limits>
#include <immintrin.h>  // ->  AVX, AVX2, FMA

#if defined(_MSC_VER) && !defined(__AVX2__)
#error "Must be compiled with option /arch:AVX2"
#endif

using namespace std;
using namespace colortwist;

template <typename tUnevenWidthHandler>
static void colorTwistRGB48_AVX3_Generic(const void* pSrc, size_t widthOver8, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    static const __m256i shuffleConst256_12 = _mm256_setr_epi8(0, 1, 6, 7, 12, 13, 2, 3, 8, 9, 14, 15, 4, 5, 10, 11, 2, 3, 8, 9, 14, 15, 4, 5, 10, 11, 0, 1, 6, 7, 12, 13);
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

    tUnevenWidthHandler handler{ twistMatrix };

    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pSrc) + y * strideSrc);
        uint16_t* d = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pDst) + y * strideDst);
        for (size_t x = 0; x < widthOver8; ++x)
        {
            const __m256i src1src2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));     // load 16 words -> R1 G1 B1 R2 | G2 B2 R3 G3 | B3 R4 G4 B4 | R5 G5 B5 R6
            const __m256i src1src2Shuffled = _mm256_shuffle_epi8(src1src2, shuffleConst256_12);   // shuffle to       R1 R2 R3 G1 | G2 G3 B1 B2 | R4 R5 R6 G4 | G5 B3 B4 B5
            const __m128i src1Shuffled = _mm256_castsi256_si128(src1src2Shuffled);
            const __m128i src2Shuffled = _mm256_extracti128_si256(src1src2Shuffled, 1);
            const __m128i src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 16));       // load 8 words -> G6 B6 R7 G7 | B7 R8 G8 B8
            const __m128i src3Shuffled = _mm_shuffle_epi8(src3, shuffleConst3);                   // shuffle to      R7 R8 G6 G7 | G8 B6 B7 B8

            // now we prepare things in that order: R1 R2 R3 R4 R5 R6 R7 R8  (and similar for green and blue)
            const __m128i redWords = _mm_blend_epi16(_mm_blend_epi16(src1Shuffled, _mm_bslli_si128(src2Shuffled, 6), 0x38), _mm_bslli_si128(src3Shuffled, 12), 0xc0);
            const __m128i greenWords = _mm_blend_epi16(_mm_blend_epi16(_mm_bsrli_si128(src1Shuffled, 6), src2Shuffled, 0x38), _mm_bslli_si128(src3Shuffled, 6), 0xe0);
            const __m128i blueWords = _mm_blend_epi16(_mm_blend_epi16(_mm_bsrli_si128(src1Shuffled, 12), _mm_bsrli_si128(src2Shuffled, 6), 0x3c), src3Shuffled, 0xe0);

            const __m256 redFloats = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(redWords));
            const __m256 greenFloats = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(greenWords));
            const __m256 blueFloats = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(blueWords));

            __m256 m1 = _mm256_fmadd_ps(redFloats, _mm256_set1_ps(t11), _mm256_set1_ps(t14));
            __m256 m2 = _mm256_fmadd_ps(greenFloats, _mm256_set1_ps(t12), m1);
            const __m256 resultR = _mm256_fmadd_ps(blueFloats, _mm256_set1_ps(t13), m2);

            m1 = _mm256_fmadd_ps(redFloats, _mm256_set1_ps(t21), _mm256_set1_ps(t24));
            m2 = _mm256_fmadd_ps(greenFloats, _mm256_set1_ps(t22), m1);
            const __m256 resultG = _mm256_fmadd_ps(blueFloats, _mm256_set1_ps(t23), m2);

            m1 = _mm256_fmadd_ps(redFloats, _mm256_set1_ps(t31), _mm256_set1_ps(t34));
            m2 = _mm256_fmadd_ps(greenFloats, _mm256_set1_ps(t32), m1);
            const __m256 resultB = _mm256_fmadd_ps(blueFloats, _mm256_set1_ps(t33), m2);

            __m256i resultInteger = _mm256_cvtps_epi32(resultR);                      // convert to int32, and then to words
            const __m256i resultRUShorts = _mm256_broadcastsi128_si256(_mm_packus_epi32(_mm256_castsi256_si128(resultInteger), _mm256_extracti128_si256(resultInteger, 1)));

            resultInteger = _mm256_cvtps_epi32(resultG);
            const __m256i resultGUShorts = _mm256_broadcastsi128_si256(_mm_packus_epi32(_mm256_castsi256_si128(resultInteger), _mm256_extracti128_si256(resultInteger, 1)));

            resultInteger = _mm256_cvtps_epi32(resultB);
            const __m256i resultBUShorts = _mm256_broadcastsi128_si256(_mm_packus_epi32(_mm256_castsi256_si128(resultInteger), _mm256_extracti128_si256(resultInteger, 1)));

            const __m256i resultRShuffled1_256 = _mm256_shuffle_epi8(resultRUShorts, shuffleConst256_1);
            const __m256i resultGShuffled2_256 = _mm256_shuffle_epi8(resultGUShorts, shuffleConst256_2);
            const __m256i resultBShuffled3_256 = _mm256_shuffle_epi8(resultBUShorts, shuffleConst256_3);
            const __m256i result256 = _mm256_or_si256(_mm256_or_si256(resultRShuffled1_256, resultGShuffled2_256), resultBShuffled3_256);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(d), result256);

            const __m128i resultRShuffled3 = _mm_shuffle_epi8(_mm256_castsi256_si128(resultRUShorts), shuffleConst10);
            const __m128i resultGShuffled3 = _mm_shuffle_epi8(_mm256_castsi256_si128(resultGUShorts), shuffleConst11);
            const __m128i resultBShuffled3 = _mm_shuffle_epi8(_mm256_castsi256_si128(resultBUShorts), shuffleConst12);
            const __m128i result3 = _mm_or_si128(_mm_or_si128(resultRShuffled3, resultGShuffled3), resultBShuffled3);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(d + 16), result3);

            p += 24;
            d += 24;
        }

        handler.DoRemainingPixels(p, d, widthRemainder);
    }
}

inline void colorTwistRGB48_AVX3_MultipleOfEight(const void* pSrc, size_t widthOver8, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    struct NullHandler
    {
        NullHandler(const float* twistMatrix) {}
        inline void DoRemainingPixels(const uint16_t* pSrc, uint16_t* pDst, size_t remainingPixels) {}
    };

    colorTwistRGB48_AVX3_Generic<NullHandler>(pSrc, widthOver8, 0, height, strideSrc, pDst, strideDst, twistMatrix);
}

inline void colorTwistRGB48_AVX3_MultipleOfEightAndRemainder(const void* pSrc, size_t widthOver8, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    class RemainingPixelsHandler
    {
    private:
        __m128 t11t21t31;
        __m128 t12t22t32;
        __m128 t13t23t33;
        __m128 t14t24t34;
    public:
        RemainingPixelsHandler() = delete;
        RemainingPixelsHandler(const float* twistMatrix) :
            t11t21t31(_mm_setr_ps(twistMatrix[0], twistMatrix[4], twistMatrix[8], 0)),
            t12t22t32(_mm_setr_ps(twistMatrix[1], twistMatrix[5], twistMatrix[9], 0)),
            t13t23t33(_mm_setr_ps(twistMatrix[2], twistMatrix[6], twistMatrix[10], 0)),
            t14t24t34(_mm_setr_ps(twistMatrix[3], twistMatrix[7], twistMatrix[11], 0))
        {}

        inline void DoRemainingPixels(const uint16_t* pSrc, uint16_t* pDst, size_t remainingPixels)
        {
            for (size_t x = 0; x < remainingPixels; ++x)
            {
                const __m128 r_src = _mm_set1_ps(pSrc[0]);
                const __m128 g_src = _mm_set1_ps(pSrc[1]);
                const __m128 b_src = _mm_set1_ps(pSrc[2]);

                __m128 result = _mm_fmadd_ps(r_src, this->t11t21t31, this->t14t24t34);
                result = _mm_fmadd_ps(g_src, this->t12t22t32, result);
                result = _mm_fmadd_ps(b_src, this->t13t23t33, result);

                const __m128i resultInteger = _mm_cvtps_epi32(result);
                const __m128i resultShort = _mm_packus_epi32(resultInteger, resultInteger);

/*                pDst[0] = resultShort.m128i_u16[0];
                pDst[1] = resultShort.m128i_u16[1];
                pDst[2] = resultShort.m128i_u16[2];*/
                pDst[0] = static_cast<uint16_t>(_mm_extract_epi16(resultShort, 0));
                pDst[1] = static_cast<uint16_t>(_mm_extract_epi16(resultShort, 1));
                pDst[2] = static_cast<uint16_t>(_mm_extract_epi16(resultShort, 2));
                /*const uint64_t _3words = _mm_cvtsi128_si64(resultShort);
                *reinterpret_cast<uint32_t*>(pDst) = static_cast<uint32_t>(_3words);
                *(pDst + 2) = static_cast<uint16_t>(_3words >> 32);*/

                pSrc += 3;
                pDst += 3;
            }
        }
    };

    colorTwistRGB48_AVX3_Generic<RemainingPixelsHandler>(pSrc, widthOver8, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
}

colortwist::StatusCode colorTwistRGB48_AVX(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    StatusCode rc = checkArgumentsRgb48(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (rc != StatusCode::OK)
    {
        return rc;
    }

    const size_t widthRemainder = width % 8;
    const size_t widthOver8 = width / 8;

    // having two versions (with and without dealing with widths not a multiple of 8) here turned out to be a little bit faster
    if (widthRemainder == 0)
    {
        colorTwistRGB48_AVX3_MultipleOfEight(pSrc, widthOver8, height, strideSrc, pDst, strideDst, twistMatrix);
    }
    else
    {
        colorTwistRGB48_AVX3_MultipleOfEightAndRemainder(pSrc, widthOver8, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
    }

    _mm256_zeroupper();
    return StatusCode::OK;
}

//-------------------------------------------------------------------------------------------------

template <typename tUnevenWidthHandler>
static void colorTwistRGB24_AVX3_Generic(const void* pSrc, size_t widthOver8, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
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

    static const __m256i shuffleConst256_1 = _mm256_setr_epi8(
        0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1, -1, 5, -1,
        -1, 6, -1, -1, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    static const __m256i shuffleConst256_2 = _mm256_setr_epi8(
        -1, 0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1, -1, 5,
        -1, -1, 6, -1, -1, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    static const __m256i shuffleConst256_3 = _mm256_setr_epi8(
        -1, -1, 0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1, -1,
        5, -1, -1, 6, -1, -1, 7, -1, -1, -1, -1, -1, -1, -1, -1);

    static const __m128i shuffleConst1 = _mm_setr_epi8(0, 3, 6, 9, 12, 15, 1, 4, 7, 10, 13, 2, 5, 8, 11, 14);
    static const __m128i shuffleConst2 = _mm_setr_epi8(2, 5, 0, 3,  6,  1, 4, 7, 0,  0,  0, 0, 0, 0,  0,  0);

    static const __m128i mask1 = _mm_setr_epi8(  0,  0,  0,  0,  0,  0, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0);
    static const __m128i mask2 = _mm_setr_epi8(  0,  0,  0,  0,  0, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0);

    static const __m256i maskStore = _mm256_setr_epi64x(-1, -1, -1, 0);

    tUnevenWidthHandler handler{ twistMatrix };

    for (size_t y = 0; y < height; ++y)
    {
        const uint8_t* p = static_cast<const uint8_t*>(pSrc) + y * strideSrc;
        uint8_t* d = static_cast<uint8_t*>(pDst) + y * strideDst;
        for (size_t x = 0; x < widthOver8; ++x)
        {
            const __m128i src1src2_128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));     // load 16 bytes -> R1 G1 B1 R2 | G2 B2 R3 G3 | B3 R4 G4 B4 | R5 G5 B5 R6
            const __m128i src3_128 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p + 16));    // load 8 bytes  -> G6 B6 R7 G7 | B7 R8 G8 B8
            const __m128i src1src2_128Shuffled = _mm_shuffle_epi8(src1src2_128, shuffleConst1);    // shuffle to       R1 R2 R3 R4 | R5 R6 G1 G2 | G3 G4 G5 B1 | B2 B3 B4 B5
            const __m128i src3_128Shuffled = _mm_shuffle_epi8(src3_128, shuffleConst2);            // shuffle to       R7 R8 G6 G7 | G8 B6 B7 B8

            //// now we prepare things in that order: R1 R2 R3 R4 R5 R6 R7 R8  (and similar for green and blue)
            const __m128i redBytes = _mm_blendv_epi8(src1src2_128Shuffled, _mm_bslli_si128(src3_128Shuffled, 6), mask1);
            const __m128i greenBytes = _mm_blendv_epi8(_mm_bsrli_si128(src1src2_128Shuffled, 6), _mm_bslli_si128(src3_128Shuffled, 3), mask2);
            const __m128i blueBytes = _mm_blendv_epi8(_mm_bsrli_si128(src1src2_128Shuffled, 11), src3_128Shuffled, mask2);

            // and convert to int32
            const __m256i redInt = _mm256_cvtepu8_epi32(redBytes);
            const __m256i greenInt = _mm256_cvtepu8_epi32(greenBytes);
            const __m256i blueInt = _mm256_cvtepu8_epi32(blueBytes);

            // and now to floats
            const __m256 redFloats = _mm256_cvtepi32_ps(redInt);
            const __m256 greenFloats = _mm256_cvtepi32_ps(greenInt);
            const __m256 blueFloats = _mm256_cvtepi32_ps(blueInt);

            __m256 m1 = _mm256_fmadd_ps(redFloats, _mm256_set1_ps(t11), _mm256_set1_ps(t14));
            __m256 m2 = _mm256_fmadd_ps(greenFloats, _mm256_set1_ps(t12), m1);
            const __m256 resultR = _mm256_fmadd_ps(blueFloats, _mm256_set1_ps(t13), m2);

            m1 = _mm256_fmadd_ps(redFloats, _mm256_set1_ps(t21), _mm256_set1_ps(t24));
            m2 = _mm256_fmadd_ps(greenFloats, _mm256_set1_ps(t22), m1);
            const __m256 resultG = _mm256_fmadd_ps(blueFloats, _mm256_set1_ps(t23), m2);

            m1 = _mm256_fmadd_ps(redFloats, _mm256_set1_ps(t31), _mm256_set1_ps(t34));
            m2 = _mm256_fmadd_ps(greenFloats, _mm256_set1_ps(t32), m1);
            const __m256 resultB = _mm256_fmadd_ps(blueFloats, _mm256_set1_ps(t33), m2);

            __m256i resultInteger = _mm256_cvtps_epi32(resultR);                      // convert to int32, and then to words, finally to bytes
            const __m128i resultRUShorts = _mm_packus_epi32(_mm256_castsi256_si128(resultInteger), _mm256_extracti128_si256(resultInteger, 1));
            const __m256i resultRBytes = _mm256_broadcastsi128_si256(_mm_packus_epi16(resultRUShorts, resultRUShorts));

            resultInteger = _mm256_cvtps_epi32(resultG);
            const __m128i resultGUShorts = _mm_packus_epi32(_mm256_castsi256_si128(resultInteger), _mm256_extracti128_si256(resultInteger, 1));
            const __m256i resultGBytes = _mm256_broadcastsi128_si256(_mm_packus_epi16(resultGUShorts, resultGUShorts));

            resultInteger = _mm256_cvtps_epi32(resultB);
            const __m128i resultBUShorts = _mm_packus_epi32(_mm256_castsi256_si128(resultInteger), _mm256_extracti128_si256(resultInteger, 1));
            const __m256i resultBBytes = _mm256_broadcastsi128_si256(_mm_packus_epi16(resultBUShorts, resultBUShorts));

            const __m256i resultRShuffled_256 = _mm256_shuffle_epi8(resultRBytes, shuffleConst256_1);
            const __m256i resultGShuffled_256 = _mm256_shuffle_epi8(resultGBytes, shuffleConst256_2);
            const __m256i resultBShuffled_256 = _mm256_shuffle_epi8(resultBBytes, shuffleConst256_3);
            const __m256i result256 = _mm256_or_si256(_mm256_or_si256(resultRShuffled_256, resultGShuffled_256), resultBShuffled_256);

            _mm256_maskstore_epi64(reinterpret_cast<long long*>(d), maskStore, result256);

            p += 24;
            d += 24;
        }

        handler.DoRemainingPixels(p, d, widthRemainder);
    }
}

inline void colorTwistRGB24_AVX3_MultipleOfEight(const void* pSrc, size_t widthOver8, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    struct NullHandler
    {
        NullHandler(const float* twistMatrix) {}
        inline void DoRemainingPixels(const uint8_t* pSrc, uint8_t* pDst, size_t remainingPixels) {}
    };

    colorTwistRGB24_AVX3_Generic<NullHandler>(pSrc, widthOver8, 0, height, strideSrc, pDst, strideDst, twistMatrix);
}

inline void colorTwistRGB24_AVX3_MultipleOfEightAndRemainder(const void* pSrc, size_t widthOver8, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    class RemainingPixelsHandler
    {
    private:
        __m128 t11t21t31;
        __m128 t12t22t32;
        __m128 t13t23t33;
        __m128 t14t24t34;
        __m128i storeMask;
    public:
        RemainingPixelsHandler() = delete;
        RemainingPixelsHandler(const float* twistMatrix) :
            t11t21t31(_mm_setr_ps(twistMatrix[0], twistMatrix[4], twistMatrix[8], 0)),
            t12t22t32(_mm_setr_ps(twistMatrix[1], twistMatrix[5], twistMatrix[9], 0)),
            t13t23t33(_mm_setr_ps(twistMatrix[2], twistMatrix[6], twistMatrix[10], 0)),
            t14t24t34(_mm_setr_ps(twistMatrix[3], twistMatrix[7], twistMatrix[11], 0)),
            storeMask{ _mm_setr_epi8(-1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) }
        {}

        inline void DoRemainingPixels(const uint8_t* pSrc, uint8_t* pDst, size_t remainingPixels)
        {
            for (size_t x = 0; x < remainingPixels; ++x)
            {
                const __m128 r_src = _mm_set1_ps(pSrc[0]);
                const __m128 g_src = _mm_set1_ps(pSrc[1]);
                const __m128 b_src = _mm_set1_ps(pSrc[2]);

                __m128 result = _mm_fmadd_ps(r_src, this->t11t21t31, this->t14t24t34);
                result = _mm_fmadd_ps(g_src, this->t12t22t32, result);
                result = _mm_fmadd_ps(b_src, this->t13t23t33, result);

                const __m128i resultInteger = _mm_cvtps_epi32(result);
                const __m128i resultShort = _mm_packus_epi32(resultInteger, resultInteger);
                const __m128i resultByte = _mm_packus_epi16(resultShort, resultShort);

                _mm_maskmoveu_si128(resultByte, this->storeMask, reinterpret_cast<char*>(pDst));

                pSrc += 3;
                pDst += 3;
            }
        }
    };

    colorTwistRGB24_AVX3_Generic<RemainingPixelsHandler>(pSrc, widthOver8, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
}

colortwist::StatusCode colorTwistRGB24_AVX(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    StatusCode rc = checkArgumentsRgb24(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (rc != StatusCode::OK)
    {
        return rc;
    }

    const size_t widthRemainder = width % 8;
    const size_t widthOver8 = width / 8;

    if (widthRemainder == 0)
    {
        colorTwistRGB24_AVX3_MultipleOfEight(pSrc, widthOver8, height, strideSrc, pDst, strideDst, twistMatrix);
    }
    else
    {
        colorTwistRGB24_AVX3_MultipleOfEightAndRemainder(pSrc, widthOver8, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
    }

    _mm256_zeroupper();
    return StatusCode::OK;
}

#endif
