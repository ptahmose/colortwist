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
    // -------------------------------------------------------------------------
    // 1. Pre-calculation & Bias Setup
    // -------------------------------------------------------------------------
    // We add 0.5f to the translation part of the matrix (offsets).
    // Why? Standard float->int conversion truncates (rounds toward zero).
    // By pre-adding 0.5, the operation `int(x + 0.5)` effectively becomes `round_nearest(x)`.
    // This avoids changing the global rounding mode (CSR), which is slow.
    const float bias = 0.5f;
    const float off_r = twistMatrix[3] + bias;
    const float off_g = twistMatrix[7] + bias;
    const float off_b = twistMatrix[11] + bias;

    // Load rotation/scaling coefficients into scalar float registers
    const float c0 = twistMatrix[0], c1 = twistMatrix[1], c2 = twistMatrix[2];
    const float c4 = twistMatrix[4], c5 = twistMatrix[5], c6 = twistMatrix[6];
    const float c8 = twistMatrix[8], c9 = twistMatrix[9], c10 = twistMatrix[10];

    const uint8_t* srcRow = static_cast<const uint8_t*>(pSrc);
    uint8_t* dstRow = static_cast<uint8_t*>(pDst);

    // Stride is 3 bytes because pixels are packed as [R, G, B, R, G, B...]
    const ptrdiff_t rgb24_stride = 3;

    for (size_t y = 0; y < height; ++y)
    {
        const uint8_t* ptrSrc = srcRow;
        uint8_t* ptrDst = dstRow;
        size_t w = width; // Remaining pixels to process in this row
        size_t vl;        // Vector Length (pixels processed in current iteration)

        // ---------------------------------------------------------------------
        // Inner Loop: Process 'vl' pixels at a time
        // ---------------------------------------------------------------------
        for (; w > 0; w -= vl, ptrSrc += 3 * vl, ptrDst += 3 * vl)
        {
            // -----------------------------------------------------------------
            // 2. Vector Length Configuration
            // -----------------------------------------------------------------
            // We request a vector length for 32-bit elements (float) with LMUL=4.
            // LMUL=4 groups 4 vector registers together, effectively quadrupling 
            // the vector size. This maximizes throughput by processing more pixels per instruction.
            // e.g., on 128-bit hardware, LMUL=4 means we process 16 floats (pixels) per instruction.
            vl = __riscv_vsetvl_e32m4(w);

            // -----------------------------------------------------------------
            // 3. Strided Load (De-interleaving)
            // -----------------------------------------------------------------
            // Instead of loading a chunk of RGBRGB... and shuffling, we use "Strided Load".
            // - Load R: start at ptrSrc+0, skip 3 bytes every time.
            // - Load G: start at ptrSrc+1, skip 3 bytes every time.
            // - Load B: start at ptrSrc+2, skip 3 bytes every time.
            // Result: Three separate vectors (Planar R, Planar G, Planar B).
            // Note: Input is U8, so we use `vlse8`.
            vuint8m1_t v_r8 = __riscv_vlse8_v_u8m1(ptrSrc + 0, rgb24_stride, vl);
            vuint8m1_t v_g8 = __riscv_vlse8_v_u8m1(ptrSrc + 1, rgb24_stride, vl);
            vuint8m1_t v_b8 = __riscv_vlse8_v_u8m1(ptrSrc + 2, rgb24_stride, vl);

            // -----------------------------------------------------------------
            // 4. Widening Conversions (U8 -> U16 -> F32)
            // -----------------------------------------------------------------
            // Step A: Zero-Extend U8 to U16.
            // We use `vzext` (Vector Zero Extend).
            // Input LMUL=1 (U8), Output LMUL=2 (U16). 
            vuint16m2_t v_r16 = __riscv_vzext_vf2_u16m2(v_r8, vl);
            vuint16m2_t v_g16 = __riscv_vzext_vf2_u16m2(v_g8, vl);
            vuint16m2_t v_b16 = __riscv_vzext_vf2_u16m2(v_b8, vl);

            // Step B: Convert U16 integers to Float32.
            // We use `vfwcvt` (Vector Floating-Point Widening Convert).
            // Input LMUL=2 (U16), Output LMUL=4 (F32).
            vfloat32m4_t v_rf = __riscv_vfwcvt_f_xu_v_f32m4(v_r16, vl);
            vfloat32m4_t v_gf = __riscv_vfwcvt_f_xu_v_f32m4(v_g16, vl);
            vfloat32m4_t v_bf = __riscv_vfwcvt_f_xu_v_f32m4(v_b16, vl);

            // -----------------------------------------------------------------
            // 5. Matrix Multiplication (FMA)
            // -----------------------------------------------------------------
            // We initialize accumulators with the pre-biased offsets.
            // Use `vfmv` (Vector Floating-Point Move).
            vfloat32m4_t v_dst_r = __riscv_vfmv_v_f_f32m4(off_r, vl);
            vfloat32m4_t v_dst_g = __riscv_vfmv_v_f_f32m4(off_g, vl);
            vfloat32m4_t v_dst_b = __riscv_vfmv_v_f_f32m4(off_b, vl);

            // Perform Fused Multiply-Accumulate (FMA).
            // dst = dst + (src * scalar_coeff)
            // Red Channel Calculation
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c0, v_rf, vl);
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c1, v_gf, vl);
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c2, v_bf, vl);

            // Green Channel Calculation
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c4, v_rf, vl);
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c5, v_gf, vl);
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c6, v_bf, vl);

            // Blue Channel Calculation
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c8, v_rf, vl);
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c9, v_gf, vl);
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c10, v_bf, vl);

            // -----------------------------------------------------------------
            // 6. Conversion back to Integer (Truncate)
            // -----------------------------------------------------------------
            // Convert Float32 to Uint32.
            // `rtz` = Round Toward Zero (Truncate).
            // Since we added +0.5 bias earlier, Truncate(x+0.5) is mathematically identical to RoundNearest(x).
            vuint32m4_t v_ri32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_r, vl);
            vuint32m4_t v_gi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_g, vl);
            vuint32m4_t v_bi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_b, vl);

            // -----------------------------------------------------------------
            // 7. Narrowing & Saturation
            // -----------------------------------------------------------------
            // We need to pack 32-bit integers back to 8-bit.
            // `vnclipu`: Vector Narrowing Clip (Unsigned).
            // It performs: dst = saturate(src >> shift)

            // Step A: U32 -> U16 (Saturate to 0xFFFF)
            // We use shift=0. Values > 65535 are clamped.
            vuint16m2_t v_ri16 = __riscv_vnclipu_wx_u16m2(v_ri32, 0, vl);
            vuint16m2_t v_gi16 = __riscv_vnclipu_wx_u16m2(v_gi32, 0, vl);
            vuint16m2_t v_bi16 = __riscv_vnclipu_wx_u16m2(v_bi32, 0, vl);

            // Step B: U16 -> U8 (Saturate to 0xFF)
            // Values > 255 are clamped. Result is back in 8-bit.
            v_r8 = __riscv_vnclipu_wx_u8m1(v_ri16, 0, vl);
            v_g8 = __riscv_vnclipu_wx_u8m1(v_gi16, 0, vl);
            v_b8 = __riscv_vnclipu_wx_u8m1(v_bi16, 0, vl);

            // -----------------------------------------------------------------
            // 8. Strided Store (Re-interleaving)
            // -----------------------------------------------------------------
            // Write the planar data back into interleaved format [RGB RGB...]
            // - Store R at ptrDst+0, stride 3.
            // - Store G at ptrDst+1, stride 3.
            // - Store B at ptrDst+2, stride 3.
            __riscv_vsse8_v_u8m1(ptrDst + 0, rgb24_stride, v_r8, vl);
            __riscv_vsse8_v_u8m1(ptrDst + 1, rgb24_stride, v_g8, vl);
            __riscv_vsse8_v_u8m1(ptrDst + 2, rgb24_stride, v_b8, vl);
        }

        srcRow += strideSrc;
        dstRow += strideDst;
    }
    return colortwist::StatusCode::OK;
}

#endif