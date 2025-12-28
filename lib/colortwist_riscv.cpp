#include "colortwist_config.h"
#if COLORTWISTLIB_HAS_RISCV_VECTOREXTENSIONS
#include <cstdint>
#include "colortwist_riscv.h"
#include <riscv_vector.h>

using namespace std;
using namespace colortwist;

// =============================================================================
// Function: colorTwistRGB48_RISCV
// Format:   16-bit Unsigned Integer per channel (RGB161616)
// -----------------------------------------------------------------------------
// Logic is identical to the RGB24 version, but handles 16-bit data types.
// Stride becomes 6 bytes (3 channels * 2 bytes).
// =============================================================================
colortwist::StatusCode colorTwistRGB48_RISCV(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    // Bias for rounding (same logic as above)
    const float bias = 0.5f;
    const float off_r = twistMatrix[3] + bias;
    const float off_g = twistMatrix[7] + bias;
    const float off_b = twistMatrix[11] + bias;

    // Coefficients
    const float c0 = twistMatrix[0], c1 = twistMatrix[1], c2 = twistMatrix[2];
    const float c4 = twistMatrix[4], c5 = twistMatrix[5], c6 = twistMatrix[6];
    const float c8 = twistMatrix[8], c9 = twistMatrix[9], c10 = twistMatrix[10];

    const uint8_t* srcRow = static_cast<const uint8_t*>(pSrc);
    uint8_t* dstRow = static_cast<uint8_t*>(pDst);

    // Stride for RGB48: 3 channels * sizeof(uint16_t) = 6 bytes.
    // NOTE: Strided load intrinsics usually take stride in BYTES.
    const ptrdiff_t rgb48_stride = 6;

    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* ptrSrc = reinterpret_cast<const uint16_t*>(srcRow);
        uint16_t* ptrDst = reinterpret_cast<uint16_t*>(dstRow);
        size_t w = width;
        size_t vl;

        for (; w > 0; w -= vl, ptrSrc += 3 * vl, ptrDst += 3 * vl)
        {
            // Same vector length logic (driven by float32 math requirements)
            vl = __riscv_vsetvl_e32m4(w);

            // -----------------------------------------------------------------
            // 1. Strided Load (U16)
            // -----------------------------------------------------------------
            // We load 16-bit elements. 
            // Note that `vlse16` expects the stride in BYTES (so 6, not 3).
            // Input LMUL=2 (U16) to match the element count of LMUL=4 (F32).
            vuint16m2_t v_r16 = __riscv_vlse16_v_u16m2(ptrSrc + 0, rgb48_stride, vl);
            vuint16m2_t v_g16 = __riscv_vlse16_v_u16m2(ptrSrc + 1, rgb48_stride, vl);
            vuint16m2_t v_b16 = __riscv_vlse16_v_u16m2(ptrSrc + 2, rgb48_stride, vl);

            // -----------------------------------------------------------------
            // 2. Widening (U16 -> F32)
            // -----------------------------------------------------------------
            // Direct widening from 16-bit Int to 32-bit Float.
            vfloat32m4_t v_rf = __riscv_vfwcvt_f_xu_v_f32m4(v_r16, vl);
            vfloat32m4_t v_gf = __riscv_vfwcvt_f_xu_v_f32m4(v_g16, vl);
            vfloat32m4_t v_bf = __riscv_vfwcvt_f_xu_v_f32m4(v_b16, vl);

            // -----------------------------------------------------------------
            // 3. Matrix Math
            // -----------------------------------------------------------------
            vfloat32m4_t v_dst_r = __riscv_vfmv_v_f_f32m4(off_r, vl);
            vfloat32m4_t v_dst_g = __riscv_vfmv_v_f_f32m4(off_g, vl);
            vfloat32m4_t v_dst_b = __riscv_vfmv_v_f_f32m4(off_b, vl);

            // Red
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c0, v_rf, vl);
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c1, v_gf, vl);
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c2, v_bf, vl);

            // Green
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c4, v_rf, vl);
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c5, v_gf, vl);
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c6, v_bf, vl);

            // Blue
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c8, v_rf, vl);
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c9, v_gf, vl);
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c10, v_bf, vl);

            // -----------------------------------------------------------------
            // 4. Convert & Saturate
            // -----------------------------------------------------------------
            // Float32 -> Uint32 (Truncate)
            vuint32m4_t v_ri32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_r, vl);
            vuint32m4_t v_gi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_g, vl);
            vuint32m4_t v_bi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_b, vl);

            // U32 -> U16 (Saturate)
            // Result goes back into v_r16. Clamps values > 65535.
            v_r16 = __riscv_vnclipu_wx_u16m2(v_ri32, 0, vl);
            v_g16 = __riscv_vnclipu_wx_u16m2(v_gi32, 0, vl);
            v_b16 = __riscv_vnclipu_wx_u16m2(v_bi32, 0, vl);

            // -----------------------------------------------------------------
            // 5. Strided Store (U16)
            // -----------------------------------------------------------------
            __riscv_vsse16_v_u16m2(ptrDst + 0, rgb48_stride, v_r16, vl);
            __riscv_vsse16_v_u16m2(ptrDst + 1, rgb48_stride, v_g16, vl);
            __riscv_vsse16_v_u16m2(ptrDst + 2, rgb48_stride, v_b16, vl);
        }

        srcRow += strideSrc;
        dstRow += strideDst;
    }

    return colortwist::StatusCode::OK;
}
// =============================================================================
// Function: colorTwistRGB24_RISCV
// Format:   8-bit Unsigned Integer per channel (RGB888)
// -----------------------------------------------------------------------------
// This function applies a 3x4 color matrix to an RGB image.
// It uses RISC-V Vector (RVV) intrinsics with "Strided Loads" to handle
// the interleaved RGB data structure without needing complex shuffle instructions
// or tuple types that caused compilation errors on GCC 13.
// =============================================================================
colortwist::StatusCode colorTwistRGB24_RISCV(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    // --- Matrix Setup (Common) ---
    const float bias = 0.5f;
    const float off_r = twistMatrix[3] + bias;
    const float off_g = twistMatrix[7] + bias;
    const float off_b = twistMatrix[11] + bias;

    const float c0 = twistMatrix[0], c1 = twistMatrix[1], c2 = twistMatrix[2];
    const float c4 = twistMatrix[4], c5 = twistMatrix[5], c6 = twistMatrix[6];
    const float c8 = twistMatrix[8], c9 = twistMatrix[9], c10 = twistMatrix[10];

    const uint8_t* srcRow = static_cast<const uint8_t*>(pSrc);
    uint8_t* dstRow = static_cast<uint8_t*>(pDst);

    for (size_t y = 0; y < height; ++y)
    {
        const uint8_t* ptrSrc = srcRow;
        uint8_t* ptrDst = dstRow;
        size_t w = width;
        size_t vl;

        for (; w > 0; w -= vl, ptrSrc += 3 * vl, ptrDst += 3 * vl)
        {
            vl = __riscv_vsetvl_e32m4(w);

            vuint8m1_t v_r8, v_g8, v_b8;

#if COLORTWIST_USE_RVV_TUPLES
            // =========================================================
            // GCC 14+ / RVV 1.0 (Tuple Implementation)
            // =========================================================
            // 1. Load: Returns a Tuple struct containing 3 vectors
            vuint8m1x3_t v_src_tuple = __riscv_vlseg3e8_v_u8m1x3(ptrSrc, vl);

            // 2. Extract: Use vget (index 0, 1, 2)
            // Your error log suggested this intrinsic name exists.
            v_r8 = __riscv_vget_v_u8m1x3_u8m1(v_src_tuple, 0);
            v_g8 = __riscv_vget_v_u8m1x3_u8m1(v_src_tuple, 1);
            v_b8 = __riscv_vget_v_u8m1x3_u8m1(v_src_tuple, 2);

#else
            // =========================================================
            // GCC 13 / Legacy (Strided Implementation)
            // =========================================================
            ptrdiff_t stride = 3;
            v_r8 = __riscv_vlse8_v_u8m1(ptrSrc + 0, stride, vl);
            v_g8 = __riscv_vlse8_v_u8m1(ptrSrc + 1, stride, vl);
            v_b8 = __riscv_vlse8_v_u8m1(ptrSrc + 2, stride, vl);
#endif

            // --- Math Core (Identical) ---
            vuint16m2_t v_r16 = __riscv_vzext_vf2_u16m2(v_r8, vl);
            vuint16m2_t v_g16 = __riscv_vzext_vf2_u16m2(v_g8, vl);
            vuint16m2_t v_b16 = __riscv_vzext_vf2_u16m2(v_b8, vl);

            vfloat32m4_t v_rf = __riscv_vfwcvt_f_xu_v_f32m4(v_r16, vl);
            vfloat32m4_t v_gf = __riscv_vfwcvt_f_xu_v_f32m4(v_g16, vl);
            vfloat32m4_t v_bf = __riscv_vfwcvt_f_xu_v_f32m4(v_b16, vl);

            vfloat32m4_t v_dst_r = __riscv_vfmv_v_f_f32m4(off_r, vl);
            vfloat32m4_t v_dst_g = __riscv_vfmv_v_f_f32m4(off_g, vl);
            vfloat32m4_t v_dst_b = __riscv_vfmv_v_f_f32m4(off_b, vl);

            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c0, v_rf, vl);
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c1, v_gf, vl);
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c2, v_bf, vl);

            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c4, v_rf, vl);
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c5, v_gf, vl);
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c6, v_bf, vl);

            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c8, v_rf, vl);
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c9, v_gf, vl);
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c10, v_bf, vl);

            vuint32m4_t v_ri32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_r, vl);
            vuint32m4_t v_gi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_g, vl);
            vuint32m4_t v_bi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_b, vl);

            vuint16m2_t v_ri16 = __riscv_vnclipu_wx_u16m2(v_ri32, 0, vl);
            vuint16m2_t v_gi16 = __riscv_vnclipu_wx_u16m2(v_gi32, 0, vl);
            vuint16m2_t v_bi16 = __riscv_vnclipu_wx_u16m2(v_bi32, 0, vl);

            v_r8 = __riscv_vnclipu_wx_u8m1(v_ri16, 0, vl);
            v_g8 = __riscv_vnclipu_wx_u8m1(v_gi16, 0, vl);
            v_b8 = __riscv_vnclipu_wx_u8m1(v_bi16, 0, vl);

#if COLORTWIST_USE_RVV_TUPLES
            // =========================================================
            // GCC 14+ / RVV 1.0 (Tuple Create & Store)
            // =========================================================
            // Use 'vcreate' to build the tuple in one instruction.
            // This replaces the 'vset' loop which was causing errors.
            vuint8m1x3_t v_dst_tuple = __riscv_vcreate_v_u8m1x3(v_r8, v_g8, v_b8);

            __riscv_vsseg3e8_v_u8m1x3(ptrDst, v_dst_tuple, vl);

#else
            // =========================================================
            // GCC 13 / Legacy (Strided Store)
            // =========================================================
            stride = 3;
            __riscv_vsse8_v_u8m1(ptrDst + 0, stride, v_r8, vl);
            __riscv_vsse8_v_u8m1(ptrDst + 1, stride, v_g8, vl);
            __riscv_vsse8_v_u8m1(ptrDst + 2, stride, v_b8, vl);
#endif
        }
        srcRow += strideSrc;
        dstRow += strideDst;
    }
    return colortwist::StatusCode::OK;
}

#endif