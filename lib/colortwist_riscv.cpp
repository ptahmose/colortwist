#include "colortwist_config.h"
#if COLORTWISTLIB_HAS_RISCV_VECTOREXTENSIONS
#include <cstdint>
#include "colortwist_riscv.h"
#include <riscv_vector.h>

using namespace std;
using namespace colortwist;

// RGB48 = 3 * uint16_t per pixel (RGBRGB...)
StatusCode colorTwistRGB48_RISCV(const void* pSrc,
    uint32_t width,
    uint32_t height,
    int strideSrc,
    void* pDst,
    int strideDst,
    const float* twistMatrix)
{
    // Hoist matrix scalars (row-major 3x4)
    const float m00 = twistMatrix[0], m01 = twistMatrix[1], m02 = twistMatrix[2], b0 = twistMatrix[3];
    const float m10 = twistMatrix[4], m11 = twistMatrix[5], m12 = twistMatrix[6], b1 = twistMatrix[7];
    const float m20 = twistMatrix[8], m21 = twistMatrix[9], m22 = twistMatrix[10], b2 = twistMatrix[11];

    constexpr std::ptrdiff_t kElemStrideBytes = 3 * static_cast<std::ptrdiff_t>(sizeof(std::uint16_t)); // 6 bytes

    for (uint32_t y = 0; y < height; ++y)
    {
        const std::uint8_t* rowSrcBytes =
            static_cast<const std::uint8_t*>(pSrc) + static_cast<std::ptrdiff_t>(y) * strideSrc;
        std::uint8_t* rowDstBytes =
            static_cast<std::uint8_t*>(pDst) + static_cast<std::ptrdiff_t>(y) * strideDst;

        const std::uint16_t* ps = reinterpret_cast<const std::uint16_t*>(rowSrcBytes);
        std::uint16_t* pd = reinterpret_cast<std::uint16_t*>(rowDstBytes);

        uint32_t x = 0;
        while (x < width)
        {
            // IMPORTANT: set vtype for the compute vectors (f32m2 => e32,m2)
            size_t vl = __riscv_vsetvl_e32m2(width - x);

            const std::uint16_t* base = ps + static_cast<std::ptrdiff_t>(x) * 3;

            // Strided loads for interleaved RGB
            vuint16m1_t r16 = __riscv_vlse16_v_u16m1(base + 0, kElemStrideBytes, vl);
            vuint16m1_t g16 = __riscv_vlse16_v_u16m1(base + 1, kElemStrideBytes, vl);
            vuint16m1_t b16 = __riscv_vlse16_v_u16m1(base + 2, kElemStrideBytes, vl);

            // u16 -> u32 (widen) -> f32
            vuint32m2_t r32 = __riscv_vwcvtu_x_x_v_u32m2(r16, vl);
            vuint32m2_t g32 = __riscv_vwcvtu_x_x_v_u32m2(g16, vl);
            vuint32m2_t b32 = __riscv_vwcvtu_x_x_v_u32m2(b16, vl);

            vfloat32m2_t vr = __riscv_vfcvt_f_xu_v_f32m2(r32, vl);
            vfloat32m2_t vg = __riscv_vfcvt_f_xu_v_f32m2(g32, vl);
            vfloat32m2_t vb = __riscv_vfcvt_f_xu_v_f32m2(b32, vl);

            // FMA chains
            vfloat32m2_t rDst = __riscv_vfmv_v_f_f32m2(b0, vl);
            rDst = __riscv_vfmacc_vf_f32m2(rDst, m00, vr, vl);
            rDst = __riscv_vfmacc_vf_f32m2(rDst, m01, vg, vl);
            rDst = __riscv_vfmacc_vf_f32m2(rDst, m02, vb, vl);

            vfloat32m2_t gDst = __riscv_vfmv_v_f_f32m2(b1, vl);
            gDst = __riscv_vfmacc_vf_f32m2(gDst, m10, vr, vl);
            gDst = __riscv_vfmacc_vf_f32m2(gDst, m11, vg, vl);
            gDst = __riscv_vfmacc_vf_f32m2(gDst, m12, vb, vl);

            vfloat32m2_t bDst = __riscv_vfmv_v_f_f32m2(b2, vl);
            bDst = __riscv_vfmacc_vf_f32m2(bDst, m20, vr, vl);
            bDst = __riscv_vfmacc_vf_f32m2(bDst, m21, vg, vl);
            bDst = __riscv_vfmacc_vf_f32m2(bDst, m22, vb, vl);

            // Clamp in float domain
            const vfloat32m2_t f0 = __riscv_vfmv_v_f_f32m2(0.0f, vl);
            const vfloat32m2_t f65535 = __riscv_vfmv_v_f_f32m2(65535.0f, vl);

            rDst = __riscv_vfmax_vv_f32m2(rDst, f0, vl);
            rDst = __riscv_vfmin_vv_f32m2(rDst, f65535, vl);

            gDst = __riscv_vfmax_vv_f32m2(gDst, f0, vl);
            gDst = __riscv_vfmin_vv_f32m2(gDst, f65535, vl);

            bDst = __riscv_vfmax_vv_f32m2(bDst, f0, vl);
            bDst = __riscv_vfmin_vv_f32m2(bDst, f65535, vl);

            // f32 -> u32 -> narrow to u16
            vuint32m2_t ro32 = __riscv_vfcvt_xu_f_v_u32m2(rDst, vl);
            vuint32m2_t go32 = __riscv_vfcvt_xu_f_v_u32m2(gDst, vl);
            vuint32m2_t bo32 = __riscv_vfcvt_xu_f_v_u32m2(bDst, vl);

            vuint16m1_t ro16 = __riscv_vnclipu_wx_u16m1(ro32, 0, vl);
            vuint16m1_t go16 = __riscv_vnclipu_wx_u16m1(go32, 0, vl);
            vuint16m1_t bo16 = __riscv_vnclipu_wx_u16m1(bo32, 0, vl);

            std::uint16_t* outBase = pd + static_cast<std::ptrdiff_t>(x) * 3;

            __riscv_vsse16_v_u16m1(outBase + 0, kElemStrideBytes, ro16, vl);
            __riscv_vsse16_v_u16m1(outBase + 1, kElemStrideBytes, go16, vl);
            __riscv_vsse16_v_u16m1(outBase + 2, kElemStrideBytes, bo16, vl);

            x += static_cast<uint32_t>(vl);
        }
    }

    return StatusCode::OK;
}

// RGB24 = 3 * uint8_t per pixel (RGBRGB...)
StatusCode colorTwistRGB24_RISCV(const void* pSrc,
    uint32_t width,
    uint32_t height,
    int strideSrc,
    void* pDst,
    int strideDst,
    const float* twistMatrix)
{
    // Hoist matrix scalars
    const float m00 = twistMatrix[0], m01 = twistMatrix[1], m02 = twistMatrix[2], b0 = twistMatrix[3];
    const float m10 = twistMatrix[4], m11 = twistMatrix[5], m12 = twistMatrix[6], b1 = twistMatrix[7];
    const float m20 = twistMatrix[8], m21 = twistMatrix[9], m22 = twistMatrix[10], b2 = twistMatrix[11];

    const std::ptrdiff_t elemStrideBytes = 3; // 3 bytes per pixel

    for (uint32_t y = 0; y < height; ++y)
    {
        const std::uint8_t* rowSrcBytes =
            static_cast<const std::uint8_t*>(pSrc) + static_cast<std::ptrdiff_t>(y) * strideSrc;
        std::uint8_t* rowDstBytes =
            static_cast<std::uint8_t*>(pDst) + static_cast<std::ptrdiff_t>(y) * strideDst;

        const std::uint8_t* ps = reinterpret_cast<const std::uint8_t*>(rowSrcBytes);
        std::uint8_t* pd = reinterpret_cast<std::uint8_t*>(rowDstBytes);

        uint32_t x = 0;
        while (x < width)
        {
            // Pick VL for 8-bit elements.
            size_t vl = __riscv_vsetvl_e8m1(width - x);

            const std::uint8_t* base = ps + static_cast<std::ptrdiff_t>(x) * 3;

            // Strided loads of interleaved RGB
            vuint8m1_t r8 = __riscv_vlse8_v_u8m1(base + 0, elemStrideBytes, vl);
            vuint8m1_t g8 = __riscv_vlse8_v_u8m1(base + 1, elemStrideBytes, vl);
            vuint8m1_t b8 = __riscv_vlse8_v_u8m1(base + 2, elemStrideBytes, vl);

            // Widen u8 -> u16 -> u32, then convert to f32
            vuint16m2_t r16 = __riscv_vwcvtu_x_x_v_u16m2(r8, vl);
            vuint16m2_t g16 = __riscv_vwcvtu_x_x_v_u16m2(g8, vl);
            vuint16m2_t b16 = __riscv_vwcvtu_x_x_v_u16m2(b8, vl);

            vuint32m4_t r32 = __riscv_vwcvtu_x_x_v_u32m4(r16, vl);
            vuint32m4_t g32 = __riscv_vwcvtu_x_x_v_u32m4(g16, vl);
            vuint32m4_t b32 = __riscv_vwcvtu_x_x_v_u32m4(b16, vl);

            vfloat32m4_t vr = __riscv_vfcvt_f_xu_v_f32m4(r32, vl);
            vfloat32m4_t vg = __riscv_vfcvt_f_xu_v_f32m4(g32, vl);
            vfloat32m4_t vb = __riscv_vfcvt_f_xu_v_f32m4(b32, vl);

            // FMACC chains
            vfloat32m4_t rDst = __riscv_vfmv_v_f_f32m4(b0, vl);
            rDst = __riscv_vfmacc_vf_f32m4(rDst, m00, vr, vl);
            rDst = __riscv_vfmacc_vf_f32m4(rDst, m01, vg, vl);
            rDst = __riscv_vfmacc_vf_f32m4(rDst, m02, vb, vl);

            vfloat32m4_t gDst = __riscv_vfmv_v_f_f32m4(b1, vl);
            gDst = __riscv_vfmacc_vf_f32m4(gDst, m10, vr, vl);
            gDst = __riscv_vfmacc_vf_f32m4(gDst, m11, vg, vl);
            gDst = __riscv_vfmacc_vf_f32m4(gDst, m12, vb, vl);

            vfloat32m4_t bDst = __riscv_vfmv_v_f_f32m4(b2, vl);
            bDst = __riscv_vfmacc_vf_f32m4(bDst, m20, vr, vl);
            bDst = __riscv_vfmacc_vf_f32m4(bDst, m21, vg, vl);
            bDst = __riscv_vfmacc_vf_f32m4(bDst, m22, vb, vl);

            // Clamp to [0..255]
            const vfloat32m4_t f0 = __riscv_vfmv_v_f_f32m4(0.0f, vl);
            const vfloat32m4_t f255 = __riscv_vfmv_v_f_f32m4(255.0f, vl);

            rDst = __riscv_vfmax_vv_f32m4(rDst, f0, vl);
            rDst = __riscv_vfmin_vv_f32m4(rDst, f255, vl);

            gDst = __riscv_vfmax_vv_f32m4(gDst, f0, vl);
            gDst = __riscv_vfmin_vv_f32m4(gDst, f255, vl);

            bDst = __riscv_vfmax_vv_f32m4(bDst, f0, vl);
            bDst = __riscv_vfmin_vv_f32m4(bDst, f255, vl);

            // Convert back: f32 -> u32 -> narrow to u8
            vuint32m4_t ro32 = __riscv_vfcvt_xu_f_v_u32m4(rDst, vl);
            vuint32m4_t go32 = __riscv_vfcvt_xu_f_v_u32m4(gDst, vl);
            vuint32m4_t bo32 = __riscv_vfcvt_xu_f_v_u32m4(bDst, vl);

            vuint16m2_t ro16 = __riscv_vnclipu_wx_u16m2(ro32, 0, vl);
            vuint16m2_t go16 = __riscv_vnclipu_wx_u16m2(go32, 0, vl);
            vuint16m2_t bo16 = __riscv_vnclipu_wx_u16m2(bo32, 0, vl);

            vuint8m1_t ro8 = __riscv_vnclipu_wx_u8m1(ro16, 0, vl);
            vuint8m1_t go8 = __riscv_vnclipu_wx_u8m1(go16, 0, vl);
            vuint8m1_t bo8 = __riscv_vnclipu_wx_u8m1(bo16, 0, vl);

            std::uint8_t* outBase = pd + static_cast<std::ptrdiff_t>(x) * 3;
            __riscv_vsse8_v_u8m1(outBase + 0, elemStrideBytes, ro8, vl);
            __riscv_vsse8_v_u8m1(outBase + 1, elemStrideBytes, go8, vl);
            __riscv_vsse8_v_u8m1(outBase + 2, elemStrideBytes, bo8, vl);

            x += static_cast<uint32_t>(vl);
        }
    }

    return StatusCode::OK;
}


#endif