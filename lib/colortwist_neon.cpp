#include "colortwist.h"
#include "colortwist_config.h"
#if COLORTWISTLIB_HASNEON

#include <cstddef>
#include <arm_neon.h>

// references:
// https://developer.arm.com/documentation/dui0491/c/using-neon-support/store-a-single-vector-or-lane?lang=en
// https://developer.arm.com/architectures/instruction-sets/simd-isas/neon/neon-programmers-guide-for-armv8-a/coding-for-neon/single-page
// https://developer.arm.com/architectures/instruction-sets/simd-isas/neon/intrinsics

bool colorTwistRGB48_NEON(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    float32x4_t c0 = { twistMatrix[0],twistMatrix[4],twistMatrix[8],0 };
    float32x4_t c1 = { twistMatrix[1],twistMatrix[5],twistMatrix[9],0 };
    float32x4_t c2 = { twistMatrix[2],twistMatrix[6],twistMatrix[10],0 };
    float32x4_t c3 = { twistMatrix[3] + 0.5f,twistMatrix[7] + 0.5f,twistMatrix[11] + 0.5f,0 };

    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pSrc) + y * strideSrc);
        uint16_t* d = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pDst) + y * strideDst);
        for (size_t x = 0; x < width; ++x)
        {
            uint16x4_t data = vld1_u16(static_cast<const uint16_t*>(p));
            uint32x4_t dataInt32 = vmovl_u16(data);
            float32x4_t dataFloat = vcvtq_f32_u32(dataInt32);

            float32x4_t r = vdupq_lane_f32(vget_low_f32(dataFloat), 0);
            float32x4_t g = vdupq_lane_f32(vget_low_f32(dataFloat), 1);
            float32x4_t b = vdupq_lane_f32(vget_high_f32(dataFloat), 0);

            float32x4_t m1 = vmlaq_f32(c3, r, c0);
            float32x4_t m2 = vmlaq_f32(m1, g, c1);
            float32x4_t m3 = vmlaq_f32(m2, b, c2);

            // conversion to int with rounding -> the intrinsic is missing, was added here -> https://patchwork.ozlabs.org/project/gcc/patch/1601891882-13015-1-git-send-email-christophe.lyon@linaro.org/
            //uint32x4_t rInt = vcvtnq_u32_f32(m3);
            uint32x4_t rInt = vcvtq_u32_f32(m3);
            uint16x4_t rShort = vqmovn_u32(rInt);

            vst1_lane_u32(reinterpret_cast<uint32_t*>(d), vreinterpret_u32_u16(rShort), 0);
            vst1_lane_u16(2 + d, rShort, 2);
            p += 3;
            d += 3;
        }
    }

    return true;
}

template <typename tUnevenWidthHandler>
static bool colorTwistRGB48_NEON2_Generic(const void* pSrc, size_t widthOver4, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    // yes, we are using here 16 (of 32) qword-register - but we still have enough left for the
    //  calculation itself to be efficient (-> 16 are left)
    float32x4_t t11 = vdupq_n_f32(twistMatrix[0]);
    float32x4_t t12 = vdupq_n_f32(twistMatrix[1]);
    float32x4_t t13 = vdupq_n_f32(twistMatrix[2]);
    float32x4_t t14 = vdupq_n_f32(twistMatrix[3] + .5f);    // we add 0.5 here for rounding - because the instrinsic vcvtnq_u32_f32 is missing,
    float32x4_t t21 = vdupq_n_f32(twistMatrix[4]);          //  -> https://patchwork.ozlabs.org/project/gcc/patch/1601891882-13015-1-git-send-email-christophe.lyon@linaro.org/
    float32x4_t t22 = vdupq_n_f32(twistMatrix[5]);
    float32x4_t t23 = vdupq_n_f32(twistMatrix[6]);
    float32x4_t t24 = vdupq_n_f32(twistMatrix[7] + .5f);
    float32x4_t t31 = vdupq_n_f32(twistMatrix[8]);
    float32x4_t t32 = vdupq_n_f32(twistMatrix[9]);
    float32x4_t t33 = vdupq_n_f32(twistMatrix[10]);
    float32x4_t t34 = vdupq_n_f32(twistMatrix[11] + .5f);

    tUnevenWidthHandler handler{ twistMatrix };

    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pSrc) + y * strideSrc);
        uint16_t* d = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pDst) + y * strideDst);
        for (size_t x = 0; x < widthOver4; ++x)
        {
            uint16x4x3_t data = vld3_u16(static_cast<const uint16_t*>(p));
            float32x4_t dataFloatR = vcvtq_f32_u32(vmovl_u16(data.val[0]));
            float32x4_t dataFloatG = vcvtq_f32_u32(vmovl_u16(data.val[1]));
            float32x4_t dataFloatB = vcvtq_f32_u32(vmovl_u16(data.val[2]));

            float32x4_t resultR = vmlaq_f32(vmlaq_f32(vmlaq_f32(t14, dataFloatR, t11), dataFloatG, t12), dataFloatB, t13);
            float32x4_t resultG = vmlaq_f32(vmlaq_f32(vmlaq_f32(t24, dataFloatR, t21), dataFloatG, t22), dataFloatB, t23);
            float32x4_t resultB = vmlaq_f32(vmlaq_f32(vmlaq_f32(t34, dataFloatR, t31), dataFloatG, t32), dataFloatB, t33);

            uint16x4_t rShortPixelR = vqmovn_u32(vcvtq_u32_f32(resultR));
            uint16x4_t rShortPixelG = vqmovn_u32(vcvtq_u32_f32(resultG));
            uint16x4_t rShortPixelB = vqmovn_u32(vcvtq_u32_f32(resultB));

            vst3_u16(d, uint16x4x3_t{ rShortPixelR,rShortPixelG,rShortPixelB });

            p += 3 * 4;
            d += 3 * 4;
        }

        handler.DoRemainingPixels(p, d, widthRemainder);
    }

    return true;
}

inline static bool colorTwistRGB48_NEON2_MultipleOfFour(const void* pSrc, size_t widthOver4, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    struct NullHandler
    {
        NullHandler(const float* twistMatrix) {}
        inline void DoRemainingPixels(const uint16_t* pSrc, uint16_t* pDst, size_t remainingPixels) {}
    };

    return colorTwistRGB48_NEON2_Generic<NullHandler>(pSrc, widthOver4, 0, height, strideSrc, pDst, strideDst, twistMatrix);
}

inline static bool colorTwistRGB48_NEON2_MultipleOfFourAndRemainder(const void* pSrc, size_t widthOver4, size_t widthRemainder, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    class RemainingPixelsHandler
    {
    private:
        float32x4_t c0;
        float32x4_t c1;
        float32x4_t c2;
        float32x4_t c3;
    public:
        RemainingPixelsHandler() = delete;
        RemainingPixelsHandler(const float* twistMatrix)
            : c0{ twistMatrix[0],twistMatrix[4],twistMatrix[8],0 },
            c1{ twistMatrix[1],twistMatrix[5],twistMatrix[9],0 },
            c2{ twistMatrix[2],twistMatrix[6],twistMatrix[10],0 },
            c3{ twistMatrix[3] + 0.5f,twistMatrix[7] + 0.5f,twistMatrix[11] + 0.5f,0 }    // we add 0.5 here for rounding - because the instrinsic vcvtnq_u32_f32 is missing,
        {}

        inline void DoRemainingPixels(const uint16_t* pSrc, uint16_t* pDst, size_t remainingPixels)
        {
            for (size_t i = 0; i < remainingPixels; ++i)
            {
                uint16x4_t data = vld1_u16(static_cast<const uint16_t*>(pSrc));
                uint32x4_t dataint32 = vmovl_u16(data);
                float32x4_t dataFloat = vcvtq_f32_u32(dataint32);

                float32x4_t r = vdupq_lane_f32(vget_low_f32(dataFloat), 0);
                float32x4_t g = vdupq_lane_f32(vget_low_f32(dataFloat), 1);
                float32x4_t b = vdupq_lane_f32(vget_high_f32(dataFloat), 0);
                float32x4_t m1 = vmlaq_f32(this->c3, r, this->c0);
                float32x4_t m2 = vmlaq_f32(m1, g, this->c1);
                float32x4_t m3 = vmlaq_f32(m2, b, this->c2);

                // conversion to int with rounding -> the intrinsic is missing, was added here -> https://patchwork.ozlabs.org/project/gcc/patch/1601891882-13015-1-git-send-email-christophe.lyon@linaro.org/
                //uint32x4_t rInt = vcvtnq_u32_f32(m3);
                uint32x4_t rInt = vcvtq_u32_f32(m3);
                uint16x4_t rShort = vqmovn_u32(rInt);

                vst1_lane_u32(reinterpret_cast<uint32_t*>(pDst), vreinterpret_u32_u16(rShort), 0);
                vst1_lane_u16(2 + pDst, rShort, 2);
                pSrc += 3;
                pDst += 3;
            }
        }
    };

    return colorTwistRGB48_NEON2_Generic<RemainingPixelsHandler>(pSrc, widthOver4, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
}

bool colorTwistRGB48_NEON2(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    const size_t widthRemainder = width % 4;
    const size_t widthOver4 = width / 4;

    // having two versions (with and without dealing with widths not a multiple of 4) here turned out to be a little bit faster
    if (widthRemainder == 0)
    {
        return colorTwistRGB48_NEON2_MultipleOfFour(pSrc, widthOver4, height, strideSrc, pDst, strideDst, twistMatrix);
    }
    else
    {
        return colorTwistRGB48_NEON2_MultipleOfFourAndRemainder(pSrc, widthOver4, widthRemainder, height, strideSrc, pDst, strideDst, twistMatrix);
    }
}

#endif
