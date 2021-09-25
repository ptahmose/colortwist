#include "colortwist_config.h"
#if COLORTWISTLIB_HASNEON
#include "colortwist_neon.h"
#include "colortwist_c.h"
#include "utils.h"
#include <cstddef>
#include <arm_neon.h>

// with VC-compiler, the initialization of a neon-type like "float32x2_t v{1,2}" does not work
#if _MSC_VER
 #define CANINITIALIZENEONTYPES (0)
#else 
 #define CANINITIALIZENEONTYPES (1)
#endif

#if CANINITIALIZENEONTYPES
    #define DECLARE_float32x4_t(_n,f1,f2,f3,f4)\
    float32x4_t _n {(f1),(f2),(f3),(f4)}
    #define DECLARE_float32x2_t(_n,f1,f2)\
    float32x2_t _n {(f1),(f2)}
#else
    #define DECLARE_float32x4_t(_n,f1,f2,f3,f4)\
    float32x4_t _n;_n.n128_f32[0]=(f1);_n.n128_f32[1]=(f2);_n.n128_f32[2]=(f3);_n.n128_f32[3]=(f4)
    #define DECLARE_float32x2_t(_n,f1,f2)\
    float32x2_t _n;_n.n64_f32[0]=(f1);_n.n64_f32[1]=(f2)
#endif

using namespace colortwist;

// references:
// https://developer.arm.com/documentation/dui0491/c/using-neon-support/store-a-single-vector-or-lane?lang=en
// https://developer.arm.com/architectures/instruction-sets/simd-isas/neon/neon-programmers-guide-for-armv8-a/coding-for-neon/single-page
// https://developer.arm.com/architectures/instruction-sets/simd-isas/neon/intrinsics
// https://github.com/thenifty/neon-guide

static inline void DoOnePixelRgb48ReadOneByteBehind(const uint16_t* p, uint16_t* d, const float32x4_t& c0, const float32x4_t& c1, const float32x4_t& c2, const float32x4_t& c3)
{
    const uint16x4_t data = vld1_u16(static_cast<const uint16_t*>(p));
    const uint32x4_t dataInt32 = vmovl_u16(data);
    const float32x4_t dataFloat = vcvtq_f32_u32(dataInt32);

    const float32x4_t r = vdupq_lane_f32(vget_low_f32(dataFloat), 0);
    const float32x4_t g = vdupq_lane_f32(vget_low_f32(dataFloat), 1);
    const float32x4_t b = vdupq_lane_f32(vget_high_f32(dataFloat), 0);

    const float32x4_t m1 = vmlaq_f32(c3, r, c0);
    const float32x4_t m2 = vmlaq_f32(m1, g, c1);
    const float32x4_t m3 = vmlaq_f32(m2, b, c2);

    // conversion to int with rounding -> the intrinsic is missing, was added here -> https://patchwork.ozlabs.org/project/gcc/patch/1601891882-13015-1-git-send-email-christophe.lyon@linaro.org/
#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
    const uint16x4_t rShort = vqmovn_u32(vcvtnq_u32_f32(m3));
#else
    const uint16x4_t rShort = vqmovn_u32(vcvtq_u32_f32(m3));
#endif

    vst1_lane_u32(reinterpret_cast<uint32_t*>(d), vreinterpret_u32_u16(rShort), 0);
    vst1_lane_u16((2 + d), rShort, 2);
}

static inline void DoOnePixelRgb48ReadExact(const uint16_t* p, uint16_t* d, const float32x4_t& c0, const float32x4_t& c1, const float32x4_t& c2, const float32x4_t& c3)
{
    uint64_t _3pixels = p[0] + (((uint32_t)p[1]) << 16) + (((uint64_t)p[2]) << 32);
    const uint16x4_t data = vcreate_u16(_3pixels);
    const uint32x4_t dataInt32 = vmovl_u16(data);
    const float32x4_t dataFloat = vcvtq_f32_u32(dataInt32);

    const float32x4_t r = vdupq_lane_f32(vget_low_f32(dataFloat), 0);
    const float32x4_t g = vdupq_lane_f32(vget_low_f32(dataFloat), 1);
    const float32x4_t b = vdupq_lane_f32(vget_high_f32(dataFloat), 0);

    const float32x4_t m1 = vmlaq_f32(c3, r, c0);
    const float32x4_t m2 = vmlaq_f32(m1, g, c1);
    const float32x4_t m3 = vmlaq_f32(m2, b, c2);

    // conversion to int with rounding -> the intrinsic is missing, was added here -> https://patchwork.ozlabs.org/project/gcc/patch/1601891882-13015-1-git-send-email-christophe.lyon@linaro.org/
#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
    const uint16x4_t rShort = vqmovn_u32(vcvtnq_u32_f32(m3));
#else
    const uint16x4_t rShort = vqmovn_u32(vcvtq_u32_f32(m3));
#endif

    vst1_lane_u32(reinterpret_cast<uint32_t*>(d), vreinterpret_u32_u16(rShort), 0);
    vst1_lane_u16((2 + d), rShort, 2);
}

colortwist::StatusCode colorTwistRGB48_NEON(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    StatusCode rc = checkArgumentsRgb48(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (rc != StatusCode::OK)
    {
        return rc;
    }

    // note: if adding "const" here, this produces incorrect results? Compiler-error or am I missing something? gcc 8.3 on Raspberry4
    DECLARE_float32x4_t(c0,twistMatrix[0],twistMatrix[4],twistMatrix[8],0);
    DECLARE_float32x4_t(c1, twistMatrix[1],twistMatrix[5],twistMatrix[9],0);
    DECLARE_float32x4_t(c2, twistMatrix[2],twistMatrix[6],twistMatrix[10],0); 
#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
    DECLARE_float32x4_t(c3, twistMatrix[3],twistMatrix[7],twistMatrix[11],0 );
#else
    DECLARE_float32x4_t(c3, twistMatrix[3] + 0.5f, twistMatrix[7] + 0.5f, twistMatrix[11] + 0.5f, 0);
#endif

    for (size_t y = 0; y < height - 1; ++y)
    {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pSrc) + y * strideSrc);
        uint16_t* d = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pDst) + y * strideDst);
        for (size_t x = 0; x < width; ++x)
        {
            DoOnePixelRgb48ReadOneByteBehind(p, d, c0, c1, c2, c3);
            p += 3;
            d += 3;
        }
    }

    // for the very last pixel, we have be careful not to read beyond the source buffer
    const uint16_t* p = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pSrc) + (height - 1) * strideSrc);
    uint16_t* d = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pDst) + (height - 1) * strideDst);
    for (size_t x = 0; x < width - 1; ++x)
    {
        DoOnePixelRgb48ReadOneByteBehind(p, d, c0, c1, c2, c3);
        p += 3;
        d += 3;
    }

    DoOnePixelRgb48ReadExact(p, d, c0, c1, c2, c3);

    return StatusCode::OK;
}

template <typename tUnevenWidthHandler>
static void colorTwistRGB48_NEON2_Generic(const void* pSrc, size_t widthOver4, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    // Yes, we are using here 16 (of 32) qword-registers - but we still have enough left for the
    //  calculation itself to be efficient (-> 16 are left). This way it is faster than using "vdupq_lane_f32"
    //  in the matrix-multiplication code (from a neon-register).
    const float32x4_t t11 = vld1q_dup_f32(twistMatrix + 0);
    const float32x4_t t12 = vld1q_dup_f32(twistMatrix + 1);
    const float32x4_t t13 = vld1q_dup_f32(twistMatrix + 2);
#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
    const float32x4_t t14 = vdupq_n_f32(twistMatrix[3]);
#else
    const float32x4_t t14 = vdupq_n_f32(twistMatrix[3] + .5f);    // we add 0.5 here for rounding - because the instrinsic vcvtnq_u32_f32 is missing,
#endif
    const float32x4_t t21 = vld1q_dup_f32(twistMatrix + 4);       //  -> https://patchwork.ozlabs.org/project/gcc/patch/1601891882-13015-1-git-send-email-christophe.lyon@linaro.org/
    const float32x4_t t22 = vld1q_dup_f32(twistMatrix + 5);
    const float32x4_t t23 = vld1q_dup_f32(twistMatrix + 6);
#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
    const float32x4_t t24 = vdupq_n_f32(twistMatrix[7]);
#else
    const float32x4_t t24 = vdupq_n_f32(twistMatrix[7] + .5f);
#endif
    const float32x4_t t31 = vld1q_dup_f32(twistMatrix + 8);
    const float32x4_t t32 = vld1q_dup_f32(twistMatrix + 9);
    const float32x4_t t33 = vld1q_dup_f32(twistMatrix + 10);
#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
    const float32x4_t t34 = vdupq_n_f32(twistMatrix[11]);
#else
    const float32x4_t t34 = vdupq_n_f32(twistMatrix[11] + .5f);
#endif

    // Note: this way is a bit faster than going with "vmlaq_lane_f32" (as in the RGB24-implementation), maybe
    //        because the register pressure is higher with the RGB24-version

    tUnevenWidthHandler handler{ twistMatrix };

    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pSrc) + y * strideSrc);
        uint16_t* d = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pDst) + y * strideDst);
        for (size_t x = 0; x < widthOver4; ++x)
        {
            // this will conveniently put the data in this order into the registers:
            // [0] : R1 R2 R3 R4
            // [1] : G1 G2 G3 G4
            // [2] : B1 B2 B3 B4
            const uint16x4x3_t data = vld3_u16(p);
            const float32x4_t dataFloatR = vcvtq_f32_u32(vmovl_u16(data.val[0]));
            const float32x4_t dataFloatG = vcvtq_f32_u32(vmovl_u16(data.val[1]));
            const float32x4_t dataFloatB = vcvtq_f32_u32(vmovl_u16(data.val[2]));

            const float32x4_t resultR = vmlaq_f32(vmlaq_f32(vmlaq_f32(t14, dataFloatR, t11), dataFloatG, t12), dataFloatB, t13);
            const float32x4_t resultG = vmlaq_f32(vmlaq_f32(vmlaq_f32(t24, dataFloatR, t21), dataFloatG, t22), dataFloatB, t23);
            const float32x4_t resultB = vmlaq_f32(vmlaq_f32(vmlaq_f32(t34, dataFloatR, t31), dataFloatG, t32), dataFloatB, t33);

#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
            const uint16x4_t rShortPixelR = vqmovn_u32(vcvtnq_u32_f32(resultR));
            const uint16x4_t rShortPixelG = vqmovn_u32(vcvtnq_u32_f32(resultG));
            const uint16x4_t rShortPixelB = vqmovn_u32(vcvtnq_u32_f32(resultB));
#else
            const uint16x4_t rShortPixelR = vqmovn_u32(vcvtq_u32_f32(resultR));
            const uint16x4_t rShortPixelG = vqmovn_u32(vcvtq_u32_f32(resultG));
            const uint16x4_t rShortPixelB = vqmovn_u32(vcvtq_u32_f32(resultB));
#endif

            // and this conveniently will shuffle R, G and B into the correct order, i. e.
            // R1 G1 B1 R2 G2 B2 R3 G3 B3 R4 G4 B4
#if CANINITIALIZENEONTYPES            
            vst3_u16(d, uint16x4x3_t{ rShortPixelR,rShortPixelG,rShortPixelB });
#else            
            uint16x4x3_t shortPixels;
            shortPixels.val[0] = rShortPixelR;
            shortPixels.val[1] = rShortPixelG;
            shortPixels.val[2] = rShortPixelB;
            vst3_u16(d, shortPixels);
#endif

            p += 3 * 4;
            d += 3 * 4;
        }

        handler.DoRemainingPixels(p, d, widthRemainder);
    }
}

inline static void colorTwistRGB48_NEON2_MultipleOfFour(const void* pSrc, size_t widthOver4, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    struct NullHandler
    {
        NullHandler(const float* twistMatrix) {}
        inline void DoRemainingPixels(const uint16_t* pSrc, uint16_t* pDst, size_t remainingPixels) {}
    };

    colorTwistRGB48_NEON2_Generic<NullHandler>(pSrc, widthOver4, 0, height, strideSrc, pDst, strideDst, twistMatrix);
}

inline static void colorTwistRGB48_NEON2_MultipleOfFourAndRemainder(const void* pSrc, size_t widthOver4, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    class RemainingPixelsHandler
    {
    private:
        float32x4_t c0;
        float32x4_t c1;
        float32x4_t c2;
        float32x4_t c3;
    public:
        const bool isempty = false;
        RemainingPixelsHandler() = delete;

        RemainingPixelsHandler(const float* twistMatrix)
#if CANINITIALIZENEONTYPES
            :
            c0( twistMatrix[0],twistMatrix[4],twistMatrix[8],0 ),
            c1{ twistMatrix[1],twistMatrix[5],twistMatrix[9],0 },
            c2{ twistMatrix[2],twistMatrix[6],twistMatrix[10],0 },
#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
            c3{ twistMatrix[3],twistMatrix[7],twistMatrix[11],0 }
#else
            c3{ twistMatrix[3] + 0.5f,twistMatrix[7] + 0.5f,twistMatrix[11] + 0.5f,0.f }    // we add 0.5 here for rounding - because the intrinsic vcvtnq_u32_f32 is missing
#endif
#endif
        {
#if !CANINITIALIZENEONTYPES            
            c0.n128_f32[0]=twistMatrix[0];
            c0.n128_f32[1]=twistMatrix[4];
            c0.n128_f32[2]=twistMatrix[8];
            c0.n128_f32[3]=0;
            c1.n128_f32[0]=twistMatrix[1];
            c1.n128_f32[1]=twistMatrix[5];
            c1.n128_f32[2]=twistMatrix[9];
            c1.n128_f32[3]=0;
            c2.n128_f32[0]=twistMatrix[2];
            c2.n128_f32[1]=twistMatrix[6];
            c2.n128_f32[2]=twistMatrix[10];
            c2.n128_f32[3]=0;
#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
            c3.n128_f32[0]=twistMatrix[3];
            c3.n128_f32[1]=twistMatrix[7];
            c3.n128_f32[2]=twistMatrix[11];
#else
            c3.n128_f32[0] = twistMatrix[3] + 0.5f;
            c3.n128_f32[1] = twistMatrix[7] + 0.5f;
            c3.n128_f32[2] = twistMatrix[11] + 0.5f;
#endif
            c3.n128_f32[3]=0;
#endif            
        }

        inline void DoRemainingPixels(const uint16_t* pSrc, uint16_t* pDst, size_t remainingPixels)
        {
            for (size_t i = 0; i < remainingPixels; ++i)
            {
                DoOnePixelRgb48ReadExact(pSrc, pDst, this->c0, this->c1, this->c2, this->c3);
                pSrc += 3;
                pDst += 3;
            }
        }
    };

    colorTwistRGB48_NEON2_Generic<RemainingPixelsHandler>(pSrc, widthOver4, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
}

StatusCode colorTwistRGB48_NEON2(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    StatusCode rc = checkArgumentsRgb48(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (rc != StatusCode::OK)
    {
        return rc;
    }

    const size_t widthRemainder = width % 4;
    const size_t widthOver4 = width / 4;

    // having two versions (with and without dealing with widths not a multiple of 4) here turned out to be a little bit faster
    if (widthRemainder == 0)
    {
        colorTwistRGB48_NEON2_MultipleOfFour(pSrc, widthOver4, height, strideSrc, pDst, strideDst, twistMatrix);
    }
    else
    {
        colorTwistRGB48_NEON2_MultipleOfFourAndRemainder(pSrc, widthOver4, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
    }

    return StatusCode::OK;
}

//-------------------------------------------------------------------------------------------------

template <typename tUnevenWidthHandler>
static void colorTwistRGB24_NEON2_Generic(const void* pSrc, size_t widthOver8, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
    const float32x4_t t14 = vdupq_n_f32(twistMatrix[3]);
    const float32x4_t t24 = vdupq_n_f32(twistMatrix[7]);
    const float32x4_t t34 = vdupq_n_f32(twistMatrix[11]);
#else
    const float32x4_t t14 = vdupq_n_f32(twistMatrix[3] + .5f);    // we add 0.5 here for rounding - because the instrinsic vcvtnq_u32_f32 is missing,
    const float32x4_t t24 = vdupq_n_f32(twistMatrix[7] + .5f);    //  -> https://patchwork.ozlabs.org/project/gcc/patch/1601891882-13015-1-git-send-email-christophe.lyon@linaro.org/
    const float32x4_t t34 = vdupq_n_f32(twistMatrix[11] + .5f);
#endif

    float32x2_t c0 = vld1_f32(twistMatrix + 0);
    DECLARE_float32x2_t(c1,twistMatrix[2],twistMatrix[4]);
    float32x2_t c2 = vld1_f32(twistMatrix + 5);
    float32x2_t c3 = vld1_f32(twistMatrix + 8);
    DECLARE_float32x2_t(c4,twistMatrix[10],0);

    tUnevenWidthHandler handler{ twistMatrix };

    for (size_t y = 0; y < height; ++y)
    {
        const uint8_t* p = static_cast<const uint8_t*>(pSrc) + y * strideSrc;
        uint8_t* d = static_cast<uint8_t*>(pDst) + y * strideDst;
        for (size_t x = 0; x < widthOver8; ++x)
        {
            // this will conveniently put the data in this order into the registers:
            // [0] : R1 R2 R3 R4 R5 R6 R7 R8
            // [1] : G1 G2 G3 G4 G5 G6 G7 G8
            // [2] : B1 B2 B3 B4 B5 B6 B7 B8
            const uint8x8x3_t data = vld3_u8(p);
            const uint16x8_t dataUShortR = vmovl_u8(data.val[0]);
            const uint16x8_t dataUShortG = vmovl_u8(data.val[1]);
            const uint16x8_t dataUShortB = vmovl_u8(data.val[2]);
            const float32x4_t dataFloatR1 = vcvtq_f32_u32(vmovl_u16(vget_low_u16(dataUShortR)));
            const float32x4_t dataFloatG1 = vcvtq_f32_u32(vmovl_u16(vget_low_u16(dataUShortG)));
            const float32x4_t dataFloatB1 = vcvtq_f32_u32(vmovl_u16(vget_low_u16(dataUShortB)));
            const float32x4_t dataFloatR2 = vcvtq_f32_u32(vmovl_u16(vget_high_u16(dataUShortR)));
            const float32x4_t dataFloatG2 = vcvtq_f32_u32(vmovl_u16(vget_high_u16(dataUShortG)));
            const float32x4_t dataFloatB2 = vcvtq_f32_u32(vmovl_u16(vget_high_u16(dataUShortB)));

            const float32x4_t resultR1 = vmlaq_lane_f32(vmlaq_lane_f32(vmlaq_lane_f32(t14, dataFloatR1, c0, 0), dataFloatG1, c0, 1), dataFloatB1, c1, 0);
            const float32x4_t resultG1 = vmlaq_lane_f32(vmlaq_lane_f32(vmlaq_lane_f32(t24, dataFloatR1, c1, 1), dataFloatG1, c2, 0), dataFloatB1, c2, 1);
            const float32x4_t resultB1 = vmlaq_lane_f32(vmlaq_lane_f32(vmlaq_lane_f32(t34, dataFloatR1, c3, 0), dataFloatG1, c3, 1), dataFloatB1, c4, 0);
            const float32x4_t resultR2 = vmlaq_lane_f32(vmlaq_lane_f32(vmlaq_lane_f32(t14, dataFloatR2, c0, 0), dataFloatG2, c0, 1), dataFloatB2, c1, 0);
            const float32x4_t resultG2 = vmlaq_lane_f32(vmlaq_lane_f32(vmlaq_lane_f32(t24, dataFloatR2, c1, 1), dataFloatG2, c2, 0), dataFloatB2, c2, 1);
            const float32x4_t resultB2 = vmlaq_lane_f32(vmlaq_lane_f32(vmlaq_lane_f32(t34, dataFloatR2, c3, 0), dataFloatG2, c3, 1), dataFloatB2, c4, 0);

#if COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32
            const uint16x8_t rShortPixelR = vcombine_u16(vqmovn_u32(vcvtnq_u32_f32(resultR1)), vqmovn_u32(vcvtnq_u32_f32(resultR2)));
            const uint16x8_t rShortPixelG = vcombine_u16(vqmovn_u32(vcvtnq_u32_f32(resultG1)), vqmovn_u32(vcvtnq_u32_f32(resultG2)));
            const uint16x8_t rShortPixelB = vcombine_u16(vqmovn_u32(vcvtnq_u32_f32(resultB1)), vqmovn_u32(vcvtnq_u32_f32(resultB2)));
#else
            const uint16x8_t rShortPixelR = vcombine_u16(vqmovn_u32(vcvtq_u32_f32(resultR1)), vqmovn_u32(vcvtq_u32_f32(resultR2)));
            const uint16x8_t rShortPixelG = vcombine_u16(vqmovn_u32(vcvtq_u32_f32(resultG1)), vqmovn_u32(vcvtq_u32_f32(resultG2)));
            const uint16x8_t rShortPixelB = vcombine_u16(vqmovn_u32(vcvtq_u32_f32(resultB1)), vqmovn_u32(vcvtq_u32_f32(resultB2)));
#endif

            const uint8x8_t rBytePixelR = vqmovn_u16(rShortPixelR);
            const uint8x8_t rBytePixelG = vqmovn_u16(rShortPixelG);
            const uint8x8_t rBytePixelB = vqmovn_u16(rShortPixelB);

            // and this conveniently will shuffle R, G and B into the correct order, i. e.
            // R1 G1 B1 R2 G2 B2 R3 G3 B3 R4 G4 B4  R5 G5 B5 R6 G6 B6 R7 G7 B7 R8 G8 B8
#if CANINITIALIZENEONTYPES            
            vst3_u8(d, uint8x8x3_t{ rBytePixelR,rBytePixelG,rBytePixelB });
#else            
            uint8x8x3_t bytePixels;
            bytePixels.val[0] = rBytePixelR;
            bytePixels.val[1] = rBytePixelG;
            bytePixels.val[2] = rBytePixelB;
            vst3_u8(d, bytePixels);
#endif

            p += 3 * 8;
            d += 3 * 8;
        }

        handler.DoRemainingPixels(p, d, widthRemainder);
    }
}

inline static void colorTwistRGB24_NEON2_MultipleOfEight(const void* pSrc, size_t widthOver8, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    struct NullHandler
    {
        NullHandler(const float* twistMatrix) {}
        inline void DoRemainingPixels(const uint8_t* pSrc, uint8_t* pDst, size_t remainingPixels) {}
    };

    colorTwistRGB24_NEON2_Generic<NullHandler>(pSrc, widthOver8, 0, height, strideSrc, pDst, strideDst, twistMatrix);
}

inline static void colorTwistRGB24_NEON2_MultipleOfFourAndRemainder(const void* pSrc, size_t widthOver8, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    class RemainingPixelsHandler
    {
    private:
        const float* twistMatrix;
    public:
        RemainingPixelsHandler(const float* twistMatrix) :twistMatrix(twistMatrix) {}
        inline void DoRemainingPixels(const uint8_t* pSrc, uint8_t* pDst, size_t remainingPixels)
        {
            colorTwistRGB24_C(pSrc, remainingPixels, 1, remainingPixels, pDst, remainingPixels, this->twistMatrix);
        }
    };

    colorTwistRGB24_NEON2_Generic<RemainingPixelsHandler>(pSrc, widthOver8, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
}

StatusCode colorTwistRGB24_NEON2(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    StatusCode rc = checkArgumentsRgb24(pSrc, width, strideSrc, pDst, strideDst, twistMatrix);
    if (rc != StatusCode::OK)
    {
        return rc;
    }

    const size_t widthRemainder = width % 8;
    const size_t widthOver8 = width / 8;

    // having two versions (with and without dealing with widths not a multiple of 8) here turned out to be a little bit faster
    if (widthRemainder == 0)
    {
        colorTwistRGB24_NEON2_MultipleOfEight(pSrc, widthOver8, height, strideSrc, pDst, strideDst, twistMatrix);
    }
    else
    {
        colorTwistRGB24_NEON2_MultipleOfFourAndRemainder(pSrc, widthOver8, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
    }

    return StatusCode::OK;
}


#endif
