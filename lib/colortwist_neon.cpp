#include "colortwist.h"
#include "colortwist_config.h"
#if COLORTWISTLIB_HASNEON

#include <arm_neon.h>

bool colorTwistRGB48_NEON(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    float32x4_t c0 = { twistMatrix[0],twistMatrix[4],twistMatrix[8],0};
    float32x4_t c1 = { twistMatrix[1],twistMatrix[5],twistMatrix[9],0 };
    float32x4_t c2 = { twistMatrix[2],twistMatrix[6],twistMatrix[10],0 };
    float32x4_t c3 = { twistMatrix[3],twistMatrix[7],twistMatrix[11],0 };

    uint16x4_t data = vld1_u16((const uint16_t*)pSrc);
    uint32x4_t dataint32 = vmovl_u16(data);
    float32x4_t dataFloat = vcvtq_f32_u32(dataint32);

    float32x4_t r = vdupq_lane_f32(vget_low_f32(dataFloat), 0);
    float32x4_t g = vdupq_lane_f32(vget_low_f32(dataFloat), 1);
    float32x4_t b = vdupq_lane_f32(vget_high_f32(dataFloat), 0);
    //float32x4_t dataFloat = vcvt_f32_u16(data);
    float32x4_t m1 = vmlaq_f32(c0, r, c3);
    float32x4_t m2 = vmlaq_f32(g, c1, m1);
    float32x4_t m3 = vmlaq_f32(b, c2, m2);
    uint32x4_t rInt = vcvtq_u32_f32(m3);
    uint16x4_t rShort = vmovn_u32(rInt);

    vst1_lane_u32((uint32_t*)pDst, vreinterpret_u32_u16(rShort), 0);
    vst1_lane_u16((uint16_t*)(1+((uint32_t*)pDst)), rShort, 2);

    return true;
}


#endif