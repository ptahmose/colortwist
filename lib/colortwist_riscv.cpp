#include "colortwist_config.h"
#if COLORTWISTLIB_HAS_RISCV_VECTOREXTENSIONS
#include <cstdint>
#include "colortwist_riscv.h"
#include <riscv_vector.h>

using namespace std;
using namespace colortwist;

// Fast RVV RGB48 (RGBRGB...) color twist.
// Strategy:
// - Strided loads/stores (AoS RGB) are unavoidable without an expensive deinterleave.
// - Convert u16 -> f32 using direct widening convert (vfwcvt.f.xu.v): fewer ops than widen-to-u32 + vfcvt.
// - Use FMACC chains.
// - Clamp with vfmax/vfmin (typically efficient on RVV FP pipelines).
// - Convert back f32 -> u32 -> narrow to u16 with vnclipu (shift=0). Clamp avoids undefined out-of-range.
//
// Notes:
// - Rounding: vfcvt_xu_f truncates according to current FP rounding behavior; if you need exact
//   “round-to-nearest” semantics matching your scalar floatToInteger<uint16_t>, adjust accordingly.
// - Requires: -march=rv64gcv -mabi=lp64d (or equivalent full V support).
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

    constexpr std::ptrdiff_t kPixStrideBytes = 3 * (std::ptrdiff_t)sizeof(std::uint16_t); // 6

    for (uint32_t y = 0; y < height; ++y)
    {
        const std::uint8_t* rowSrcBytes =
            static_cast<const std::uint8_t*>(pSrc) + (std::ptrdiff_t)y * (std::ptrdiff_t)strideSrc;
        std::uint8_t* rowDstBytes =
            static_cast<std::uint8_t*>(pDst) + (std::ptrdiff_t)y * (std::ptrdiff_t)strideDst;

        const std::uint16_t* ps = reinterpret_cast<const std::uint16_t*>(rowSrcBytes);
        std::uint16_t* pd = reinterpret_cast<std::uint16_t*>(rowDstBytes);

        uint32_t x = 0;
        while (x < width)
        {
            // Choose VL for u16m1 loads/stores. We keep this VL for the whole iteration.
            // Compute uses f32m2 (widening), which matches element count.
            size_t vl = __riscv_vsetvl_e16m1(width - x);

            const std::uint16_t* base = ps + (std::ptrdiff_t)x * 3;

            // Strided loads: R at +0, G at +1, B at +2 with stride 6 bytes.
            vuint16m1_t r16 = __riscv_vlse16_v_u16m1(base + 0, kPixStrideBytes, vl);
            vuint16m1_t g16 = __riscv_vlse16_v_u16m1(base + 1, kPixStrideBytes, vl);
            vuint16m1_t b16 = __riscv_vlse16_v_u16m1(base + 2, kPixStrideBytes, vl);

            // Convert u16 -> f32 with widening convert (f32m2).
            // Using vfwcvt avoids an extra widen-to-u32 step and is usually the fastest path.
            // IMPORTANT: for f32m2 ops, vtype must be e32,m2. GCC typically emits vsetvli as needed,
            // but we keep things explicit for performance predictability.
            __riscv_vsetvl_e32m2(width - x);

            vfloat32m2_t vr = __riscv_vfwcvt_f_xu_v_f32m2(r16, vl);
            vfloat32m2_t vg = __riscv_vfwcvt_f_xu_v_f32m2(g16, vl);
            vfloat32m2_t vb = __riscv_vfwcvt_f_xu_v_f32m2(b16, vl);

            // FMACC chains: dst = bias + sum(coeff * channel)
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

            // Clamp to [0..65535] in float domain (safe; avoids undefined behavior on conversion/narrow).
            const vfloat32m2_t f0 = __riscv_vfmv_v_f_f32m2(0.0f, vl);
            const vfloat32m2_t f65535 = __riscv_vfmv_v_f_f32m2(65535.0f, vl);

            rDst = __riscv_vfmax_vv_f32m2(rDst, f0, vl);
            rDst = __riscv_vfmin_vv_f32m2(rDst, f65535, vl);
            gDst = __riscv_vfmax_vv_f32m2(gDst, f0, vl);
            gDst = __riscv_vfmin_vv_f32m2(gDst, f65535, vl);
            bDst = __riscv_vfmax_vv_f32m2(bDst, f0, vl);
            bDst = __riscv_vfmin_vv_f32m2(bDst, f65535, vl);

            // Convert to unsigned ints then narrow to u16.
            // (If your toolchain provides a direct f32->u16 saturating conversion, use it; GCC RVV usually does not.)
            vuint32m2_t ro32 = __riscv_vfcvt_xu_f_v_u32m2(rDst, vl);
            vuint32m2_t go32 = __riscv_vfcvt_xu_f_v_u32m2(gDst, vl);
            vuint32m2_t bo32 = __riscv_vfcvt_xu_f_v_u32m2(bDst, vl);

            // Switch vtype back for narrowing+stores (u16m1)
            __riscv_vsetvl_e16m1(width - x);

            vuint16m1_t ro16 = __riscv_vnclipu_wx_u16m1(ro32, 0, vl);
            vuint16m1_t go16 = __riscv_vnclipu_wx_u16m1(go32, 0, vl);
            vuint16m1_t bo16 = __riscv_vnclipu_wx_u16m1(bo32, 0, vl);

            std::uint16_t* outBase = pd + (std::ptrdiff_t)x * 3;
            __riscv_vsse16_v_u16m1(outBase + 0, kPixStrideBytes, ro16, vl);
            __riscv_vsse16_v_u16m1(outBase + 1, kPixStrideBytes, go16, vl);
            __riscv_vsse16_v_u16m1(outBase + 2, kPixStrideBytes, bo16, vl);

            x += (uint32_t)vl;
        }
    }

    return StatusCode::OK;
}

/*
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
*/
// -----------------------------------------------------------------------------
// Function: colorTwistRGB24_RISCV (uint8_t)
// Optimization: Uses Strided Loads/Stores to avoid Tuple-Type compilation errors
// -----------------------------------------------------------------------------
colortwist::StatusCode colorTwistRGB24_RISCV(const void* pSrc,
    uint32_t width,
    uint32_t height,
    int strideSrc,
    void* pDst,
    int strideDst,
    const float* twistMatrix)
{
    // 0.5 bias to match round-to-nearest when using rtz conversions
    const float off_r = twistMatrix[3] + 0.5f;
    const float off_g = twistMatrix[7] + 0.5f;
    const float off_b = twistMatrix[11] + 0.5f;

    const float c0 = twistMatrix[0], c1 = twistMatrix[1], c2 = twistMatrix[2];
    const float c4 = twistMatrix[4], c5 = twistMatrix[5], c6 = twistMatrix[6];
    const float c8 = twistMatrix[8], c9 = twistMatrix[9], c10 = twistMatrix[10];

    const std::uint8_t* __restrict srcRow = static_cast<const std::uint8_t*>(pSrc);
    std::uint8_t* __restrict dstRow = static_cast<std::uint8_t*>(pDst);

    constexpr std::ptrdiff_t rgb_stride = 3;

    for (uint32_t y = 0; y < height; ++y)
    {
        const std::uint8_t* __restrict ptrSrc = srcRow;
        std::uint8_t* __restrict ptrDst = dstRow;

        size_t w = width;
        while (w > 0)
        {
            // Set VL based on bytes (often yields maximal VL). We'll still use this vl for the whole chunk.
            const size_t vl = __riscv_vsetvl_e8m1(w);

            // Strided loads for AoS RGB
            const vuint8m1_t r8 = __riscv_vlse8_v_u8m1(ptrSrc + 0, rgb_stride, vl);
            const vuint8m1_t g8 = __riscv_vlse8_v_u8m1(ptrSrc + 1, rgb_stride, vl);
            const vuint8m1_t b8 = __riscv_vlse8_v_u8m1(ptrSrc + 2, rgb_stride, vl);

            // u8 -> u16 (zero extend, LMUL=2)
            const vuint16m2_t r16 = __riscv_vzext_vf2_u16m2(r8, vl);
            const vuint16m2_t g16 = __riscv_vzext_vf2_u16m2(g8, vl);
            const vuint16m2_t b16 = __riscv_vzext_vf2_u16m2(b8, vl);

            // u16 -> f32 (widening convert to LMUL=4)
            const vfloat32m4_t rf = __riscv_vfwcvt_f_xu_v_f32m4(r16, vl);
            const vfloat32m4_t gf = __riscv_vfwcvt_f_xu_v_f32m4(g16, vl);
            const vfloat32m4_t bf = __riscv_vfwcvt_f_xu_v_f32m4(b16, vl);

            // FMACC chains
            vfloat32m4_t dst_r = __riscv_vfmv_v_f_f32m4(off_r, vl);
            vfloat32m4_t dst_g = __riscv_vfmv_v_f_f32m4(off_g, vl);
            vfloat32m4_t dst_b = __riscv_vfmv_v_f_f32m4(off_b, vl);

            dst_r = __riscv_vfmacc_vf_f32m4(dst_r, c0, rf, vl);
            dst_r = __riscv_vfmacc_vf_f32m4(dst_r, c1, gf, vl);
            dst_r = __riscv_vfmacc_vf_f32m4(dst_r, c2, bf, vl);

            dst_g = __riscv_vfmacc_vf_f32m4(dst_g, c4, rf, vl);
            dst_g = __riscv_vfmacc_vf_f32m4(dst_g, c5, gf, vl);
            dst_g = __riscv_vfmacc_vf_f32m4(dst_g, c6, bf, vl);

            dst_b = __riscv_vfmacc_vf_f32m4(dst_b, c8, rf, vl);
            dst_b = __riscv_vfmacc_vf_f32m4(dst_b, c9, gf, vl);
            dst_b = __riscv_vfmacc_vf_f32m4(dst_b, c10, bf, vl);

            // Convert to *signed* int32 with rtz, then clamp low at 0 in integer domain.
            // High values will saturate during vnclipu.
            vint32m4_t ri = __riscv_vfcvt_rtz_x_f_v_i32m4(dst_r, vl);
            vint32m4_t gi = __riscv_vfcvt_rtz_x_f_v_i32m4(dst_g, vl);
            vint32m4_t bi = __riscv_vfcvt_rtz_x_f_v_i32m4(dst_b, vl);

            ri = __riscv_vmax_vx_i32m4(ri, 0, vl);
            gi = __riscv_vmax_vx_i32m4(gi, 0, vl);
            bi = __riscv_vmax_vx_i32m4(bi, 0, vl);

            // Reinterpret as unsigned for vnclipu
            const vuint32m4_t ru = __riscv_vreinterpret_v_i32m4_u32m4(ri);
            const vuint32m4_t gu = __riscv_vreinterpret_v_i32m4_u32m4(gi);
            const vuint32m4_t bu = __riscv_vreinterpret_v_i32m4_u32m4(bi);

            // Narrow with saturation: u32 -> u16 -> u8
            const vuint16m2_t r16n = __riscv_vnclipu_wx_u16m2(ru, 0, vl);
            const vuint16m2_t g16n = __riscv_vnclipu_wx_u16m2(gu, 0, vl);
            const vuint16m2_t b16n = __riscv_vnclipu_wx_u16m2(bu, 0, vl);

            const vuint8m1_t r8n = __riscv_vnclipu_wx_u8m1(r16n, 0, vl);
            const vuint8m1_t g8n = __riscv_vnclipu_wx_u8m1(g16n, 0, vl);
            const vuint8m1_t b8n = __riscv_vnclipu_wx_u8m1(b16n, 0, vl);

            // Strided stores
            __riscv_vsse8_v_u8m1(ptrDst + 0, rgb_stride, r8n, vl);
            __riscv_vsse8_v_u8m1(ptrDst + 1, rgb_stride, g8n, vl);
            __riscv_vsse8_v_u8m1(ptrDst + 2, rgb_stride, b8n, vl);

            ptrSrc += 3 * vl;
            ptrDst += 3 * vl;
            w -= vl;
        }

        srcRow += strideSrc;
        dstRow += strideDst;
    }

    return colortwist::StatusCode::OK;
}

#endif