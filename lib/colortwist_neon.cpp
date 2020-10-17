#include "colortwist.h"
#include "colortwist_config.h"
#if COLORTWISTLIB_HASNEON

#include <cstddef>
#include <arm_neon.h>


bool colorTwistRGB48_NEON(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    float32x4_t c0 = { twistMatrix[0],twistMatrix[4],twistMatrix[8],0 };
    float32x4_t c1 = { twistMatrix[1],twistMatrix[5],twistMatrix[9],0 };
    float32x4_t c2 = { twistMatrix[2],twistMatrix[6],twistMatrix[10],0 };
    float32x4_t c3 = { twistMatrix[3]+0.5f,twistMatrix[7] + 0.5f,twistMatrix[11] + 0.5f,0 };

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


#endif