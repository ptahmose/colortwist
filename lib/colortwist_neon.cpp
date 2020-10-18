#include "colortwist.h"
#include "colortwist_config.h"
#if COLORTWISTLIB_HASNEON

#include <cstddef>
#include <arm_neon.h>

// > https://developer.arm.com/documentation/dui0491/c/using-neon-support/store-a-single-vector-or-lane?lang=en

bool colorTwistRGB48_NEON(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    float32x4_t c0 = { twistMatrix[0],twistMatrix[4],twistMatrix[8],0 };
    float32x4_t c1 = { twistMatrix[1],twistMatrix[5],twistMatrix[9],0 };
    float32x4_t c2 = { twistMatrix[2],twistMatrix[6],twistMatrix[10],0 };
    float32x4_t c3 = { twistMatrix[3] + 0.5f,twistMatrix[7] + 0.5f,twistMatrix[11] + 0.5f,0 };

    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = (const uint16_t*)(((const uint8_t*)pSrc) + y * strideSrc);
        uint16_t* d = (uint16_t*)(((uint8_t*)pDst) + y * strideDst);
        for (size_t x = 0; x < width; ++x)
        {
            uint16x4_t data = vld1_u16((const uint16_t*)p);
            uint32x4_t dataint32 = vmovl_u16(data);
            float32x4_t dataFloat = vcvtq_f32_u32(dataint32);

            float32x4_t r = vdupq_lane_f32(vget_low_f32(dataFloat), 0);
            float32x4_t g = vdupq_lane_f32(vget_low_f32(dataFloat), 1);
            float32x4_t b = vdupq_lane_f32(vget_high_f32(dataFloat), 0);
            /*float32x4_t r = vmovq_n_f32(*p);
            float32x4_t g = vmovq_n_f32(*(p+1));
            float32x4_t b = vmovq_n_f32(*(p+2));*/
            float32x4_t m1 = vmlaq_f32(c3, r, c0);
            float32x4_t m2 = vmlaq_f32(m1, g, c1);
            float32x4_t m3 = vmlaq_f32(m2, b, c2);

            // conversion to int with rounding -> the intrinsic is missing, was added here -> https://patchwork.ozlabs.org/project/gcc/patch/1601891882-13015-1-git-send-email-christophe.lyon@linaro.org/
            //uint32x4_t rInt = vcvtnq_u32_f32(m3);
            uint32x4_t rInt = vcvtq_u32_f32(m3);
            uint16x4_t rShort = vqmovn_u32(rInt);

            vst1_lane_u32((uint32_t*)d, vreinterpret_u32_u16(rShort), 0);
            vst1_lane_u16((uint16_t*)(1 + ((uint32_t*)d)), rShort, 2);
            p += 3;
            d += 3;
        }
    }

    return true;
}

bool colorTwistRGB48_NEON2(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    float32x4_t c0 = { twistMatrix[0],twistMatrix[4],twistMatrix[8],0 };
    float32x4_t c1 = { twistMatrix[1],twistMatrix[5],twistMatrix[9],0 };
    float32x4_t c2 = { twistMatrix[2],twistMatrix[6],twistMatrix[10],0 };
    float32x4_t c3 = { twistMatrix[3] + 0.5f,twistMatrix[7] + 0.5f,twistMatrix[11] + 0.5f,0 };

    /*static const uint8x8_t tbl1{ 0,1,2,3,4,5,8,9 };
    static const uint8x8_t tbl2{ 2,3,4,5,8,9,10,11 };
    static const uint8x8_t tbl3{ 4,5,8,9,10,11,12,13 };*/
    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* p = (const uint16_t*)(((const uint8_t*)pSrc) + y * strideSrc);
        uint16_t* d = (uint16_t*)(((uint8_t*)pDst) + y * strideDst);
        for (size_t x = 0; x < width / 4; ++x)
        {
            uint16x4x3_t data = vld3_u16((const uint16_t*)p);
            uint32x4_t dataint32R = vmovl_u16(data.val[0]);
            float32x4_t dataFloatR = vcvtq_f32_u32(dataint32R);
            uint32x4_t dataint32G = vmovl_u16(data.val[1]);
            float32x4_t dataFloatG = vcvtq_f32_u32(dataint32G);
            uint32x4_t dataint32B = vmovl_u16(data.val[2]);
            float32x4_t dataFloatB = vcvtq_f32_u32(dataint32B);

            float32x4_t m1 = vmlaq_f32(c3, vdupq_lane_f32(vget_low_f32(dataFloatR), 0), c0);
            float32x4_t m2 = vmlaq_f32(m1, vdupq_lane_f32(vget_low_f32(dataFloatG), 0), c1);
            float32x4_t pixel1 = vmlaq_f32(m2, vdupq_lane_f32(vget_low_f32(dataFloatB), 0), c2);

            m1 = vmlaq_f32(c3, vdupq_lane_f32(vget_low_f32(dataFloatR), 1), c0);
            m2 = vmlaq_f32(m1, vdupq_lane_f32(vget_low_f32(dataFloatG), 1), c1);
            float32x4_t pixel2 = vmlaq_f32(m2, vdupq_lane_f32(vget_low_f32(dataFloatB), 1), c2);

            m1 = vmlaq_f32(c3, vdupq_lane_f32(vget_high_f32(dataFloatR), 0), c0);
            m2 = vmlaq_f32(m1, vdupq_lane_f32(vget_high_f32(dataFloatG), 0), c1);
            float32x4_t pixel3 = vmlaq_f32(m2, vdupq_lane_f32(vget_high_f32(dataFloatB), 0), c2);

            m1 = vmlaq_f32(c3, vdupq_lane_f32(vget_high_f32(dataFloatR), 1), c0);
            m2 = vmlaq_f32(m1, vdupq_lane_f32(vget_high_f32(dataFloatG), 1), c1);
            float32x4_t pixel4 = vmlaq_f32(m2, vdupq_lane_f32(vget_high_f32(dataFloatB), 1), c2);

            uint16x4_t rShortPixel1 = vqmovn_u32(vcvtq_u32_f32(pixel1));
            uint16x4_t rShortPixel2 = vqmovn_u32(vcvtq_u32_f32(pixel2));
            uint16x4_t rShortPixel3 = vqmovn_u32(vcvtq_u32_f32(pixel3));
            uint16x4_t rShortPixel4 = vqmovn_u32(vcvtq_u32_f32(pixel4));

            /*uint16x4x2_t t1 = vtrn_u16(rShortPixel1, rShortPixel2);
            uint16x4x2_t t2 = vtrn_u16(rShortPixel3, rShortPixel4);
            uint32x2x2_t tt = vtrn_u32(vreinterpret_u32_u16(t1.val[0]), vreinterpret_u32_u16(t2.val[0]));
            uint16x4_t x1 = vreinterpret_u16_u32(tt.val[0]);
            uint16x4_t x2 = vreinterpret_u16_u32(tt.val[1]);
            uint32x2x2_t tt2 = vtrn_u32(vreinterpret_u32_u16(t1.val[1]), vreinterpret_u32_u16(t2.val[1]));
            uint16x4_t x3 = vreinterpret_u16_u32(tt2.val[0]);
            vst3_u16(d, uint16x4x3_t{ x1,x3,x2 });*/

            /*uint16x4_t x1 = vreinterpret_u16_u8(vtbl2_u8(uint8x8x2_t{ vreinterpret_u8_u16(rShortPixel1),vreinterpret_u8_u16(rShortPixel2) }, tbl1));
            vst1_u16(d, x1);
            uint16x4_t x2 = vreinterpret_u16_u8(vtbl2_u8(uint8x8x2_t{ vreinterpret_u8_u16(rShortPixel2),vreinterpret_u8_u16(rShortPixel3) }, tbl2));
            vst1_u16(d + 4, x2);
            uint16x4_t x3 = vreinterpret_u16_u8(vtbl2_u8(uint8x8x2_t{ vreinterpret_u8_u16(rShortPixel3),vreinterpret_u8_u16(rShortPixel4) }, tbl3));
            vst1_u16(d+8, x3);*/

            vst1_lane_u32((uint32_t*)d, vreinterpret_u32_u16(rShortPixel1), 0);
            vst1_lane_u16((uint16_t*)(2 + d), rShortPixel1, 2);
            vst1_lane_u32((uint32_t*)(3 + d), vreinterpret_u32_u16(rShortPixel2), 0);
            vst1_lane_u16((uint16_t*)(5 + d), rShortPixel2, 2);
            vst1_lane_u32((uint32_t*)(6 + d), vreinterpret_u32_u16(rShortPixel3), 0);
            vst1_lane_u16((uint16_t*)(8 + d), rShortPixel3, 2);
            vst1_lane_u32((uint32_t*)(9 + d), vreinterpret_u32_u16(rShortPixel4), 0);
            vst1_lane_u16((uint16_t*)(11 + d), rShortPixel4, 2);

            p += 3 * 4;
            d += 3 * 4;
        }
    }

    return true;
}


#endif