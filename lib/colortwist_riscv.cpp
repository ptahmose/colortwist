#include "colortwist_config.h"
#if COLORTWISTLIB_HAS_RISCV_VECTOREXTENSIONS
#include <cstdint>
#include "colortwist_riscv.h"
#include <riscv_vector.h>

using namespace std;
using namespace colortwist;

// -----------------------------------------------------------------------------
// Function: colorTwistRGB48_RISCV (uint16_t)
// Optimization: Uses Strided Loads/Stores (Stride = 6 bytes)
// -----------------------------------------------------------------------------
colortwist::StatusCode colorTwistRGB48_RISCV(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    const float bias = 0.5f;
    const float off_r = twistMatrix[3] + bias;
    const float off_g = twistMatrix[7] + bias;
    const float off_b = twistMatrix[11] + bias;

    const float c0 = twistMatrix[0], c1 = twistMatrix[1], c2 = twistMatrix[2];
    const float c4 = twistMatrix[4], c5 = twistMatrix[5], c6 = twistMatrix[6];
    const float c8 = twistMatrix[8], c9 = twistMatrix[9], c10 = twistMatrix[10];

    const uint8_t* srcRow = static_cast<const uint8_t*>(pSrc);
    uint8_t* dstRow = static_cast<uint8_t*>(pDst);

    // Stride for RGB48 is 3 * sizeof(uint16_t) = 6 bytes
    const ptrdiff_t rgb48_stride = 6;

    for (size_t y = 0; y < height; ++y)
    {
        const uint16_t* ptrSrc = reinterpret_cast<const uint16_t*>(srcRow);
        uint16_t* ptrDst = reinterpret_cast<uint16_t*>(dstRow);
        size_t w = width;
        size_t vl;

        for (; w > 0; w -= vl, ptrSrc += 3 * vl, ptrDst += 3 * vl)
        {
            vl = __riscv_vsetvl_e32m4(w);

            // 1. Load Strided (U16)
            // Note: vlse16 takes stride in BYTES, not elements. Hence rgb48_stride = 6.
            vuint16m2_t v_r16 = __riscv_vlse16_v_u16m2(ptrSrc + 0, rgb48_stride, vl);
            vuint16m2_t v_g16 = __riscv_vlse16_v_u16m2(ptrSrc + 1, rgb48_stride, vl);
            vuint16m2_t v_b16 = __riscv_vlse16_v_u16m2(ptrSrc + 2, rgb48_stride, vl);

            // 2. Widen U16 -> F32 (LMUL 2 -> 4)
            vfloat32m4_t v_rf = __riscv_vfwcvt_f_xu_v_f32m4(v_r16, vl);
            vfloat32m4_t v_gf = __riscv_vfwcvt_f_xu_v_f32m4(v_g16, vl);
            vfloat32m4_t v_bf = __riscv_vfwcvt_f_xu_v_f32m4(v_b16, vl);

            // 3. Matrix Math
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

            // 4. Convert F32 -> U32 (Truncate)
            vuint32m4_t v_ri32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_r, vl);
            vuint32m4_t v_gi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_g, vl);
            vuint32m4_t v_bi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_b, vl);

            // 5. Narrow and Saturate U32 -> U16
            // Stores result back into v_r16, v_g16, v_b16
            v_r16 = __riscv_vnclipu_wx_u16m2(v_ri32, 0, vl);
            v_g16 = __riscv_vnclipu_wx_u16m2(v_gi32, 0, vl);
            v_b16 = __riscv_vnclipu_wx_u16m2(v_bi32, 0, vl);

            // 6. Store Strided
            __riscv_vsse16_v_u16m2(ptrDst + 0, rgb48_stride, v_r16, vl);
            __riscv_vsse16_v_u16m2(ptrDst + 1, rgb48_stride, v_g16, vl);
            __riscv_vsse16_v_u16m2(ptrDst + 2, rgb48_stride, v_b16, vl);
        }

        srcRow += strideSrc;
        dstRow += strideDst;
    }

    return colortwist::StatusCode::OK;
}

// -----------------------------------------------------------------------------
// Function: colorTwistRGB24_RISCV (uint8_t)
// Optimization: Uses Strided Loads/Stores to avoid Tuple-Type compilation errors
// -----------------------------------------------------------------------------
colortwist::StatusCode colorTwistRGB24_RISCV(const void* pSrc, uint32_t width, uint32_t height, int strideSrc, void* pDst, int strideDst, const float* twistMatrix)
{
    // Matrix Offsets with 0.5 rounding bias
    const float bias = 0.5f;
    const float off_r = twistMatrix[3] + bias;
    const float off_g = twistMatrix[7] + bias;
    const float off_b = twistMatrix[11] + bias;

    // Matrix Coefficients
    const float c0 = twistMatrix[0], c1 = twistMatrix[1], c2 = twistMatrix[2];
    const float c4 = twistMatrix[4], c5 = twistMatrix[5], c6 = twistMatrix[6];
    const float c8 = twistMatrix[8], c9 = twistMatrix[9], c10 = twistMatrix[10];

    const uint8_t* srcRow = static_cast<const uint8_t*>(pSrc);
    uint8_t* dstRow = static_cast<uint8_t*>(pDst);

    // Stride for RGB24 is 3 bytes (R..G..B.. -> next pixel)
    const ptrdiff_t rgb24_stride = 3;

    for (size_t y = 0; y < height; ++y)
    {
        const uint8_t* ptrSrc = srcRow;
        uint8_t* ptrDst = dstRow;
        size_t w = width;
        size_t vl;

        for (; w > 0; w -= vl, ptrSrc += 3 * vl, ptrDst += 3 * vl)
        {
            // Calculate Vector Length for 32-bit float elements (LMUL=4)
            vl = __riscv_vsetvl_e32m4(w);

            // 1. Load Strided (De-interleave manually)
            // Load R (start at offset 0, jump 3 bytes)
            vuint8m1_t v_r8 = __riscv_vlse8_v_u8m1(ptrSrc + 0, rgb24_stride, vl);
            // Load G (start at offset 1, jump 3 bytes)
            vuint8m1_t v_g8 = __riscv_vlse8_v_u8m1(ptrSrc + 1, rgb24_stride, vl);
            // Load B (start at offset 2, jump 3 bytes)
            vuint8m1_t v_b8 = __riscv_vlse8_v_u8m1(ptrSrc + 2, rgb24_stride, vl);

            // 2. Promote U8 -> U16 -> F32
            vuint16m2_t v_r16 = __riscv_vzext_vf2_u16m2(v_r8, vl);
            vuint16m2_t v_g16 = __riscv_vzext_vf2_u16m2(v_g8, vl);
            vuint16m2_t v_b16 = __riscv_vzext_vf2_u16m2(v_b8, vl);

            vfloat32m4_t v_rf = __riscv_vfwcvt_f_xu_v_f32m4(v_r16, vl);
            vfloat32m4_t v_gf = __riscv_vfwcvt_f_xu_v_f32m4(v_g16, vl);
            vfloat32m4_t v_bf = __riscv_vfwcvt_f_xu_v_f32m4(v_b16, vl);

            // 3. Matrix Math
            // Init accumulators with bias
            vfloat32m4_t v_dst_r = __riscv_vfmv_v_f_f32m4(off_r, vl);
            vfloat32m4_t v_dst_g = __riscv_vfmv_v_f_f32m4(off_g, vl);
            vfloat32m4_t v_dst_b = __riscv_vfmv_v_f_f32m4(off_b, vl);

            // Accumulate Red
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c0, v_rf, vl);
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c1, v_gf, vl);
            v_dst_r = __riscv_vfmacc_vf_f32m4(v_dst_r, c2, v_bf, vl);

            // Accumulate Green
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c4, v_rf, vl);
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c5, v_gf, vl);
            v_dst_g = __riscv_vfmacc_vf_f32m4(v_dst_g, c6, v_bf, vl);

            // Accumulate Blue
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c8, v_rf, vl);
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c9, v_gf, vl);
            v_dst_b = __riscv_vfmacc_vf_f32m4(v_dst_b, c10, v_bf, vl);

            // 4. Convert F32 -> U32 (Truncate)
            vuint32m4_t v_ri32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_r, vl);
            vuint32m4_t v_gi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_g, vl);
            vuint32m4_t v_bi32 = __riscv_vfcvt_rtz_xu_f_v_u32m4(v_dst_b, vl);

            // 5. Narrow and Saturate
            // Narrow to U16
            vuint16m2_t v_ri16 = __riscv_vnclipu_wx_u16m2(v_ri32, 0, vl);
            vuint16m2_t v_gi16 = __riscv_vnclipu_wx_u16m2(v_gi32, 0, vl);
            vuint16m2_t v_bi16 = __riscv_vnclipu_wx_u16m2(v_bi32, 0, vl);

            // Narrow to U8 (Result in v_r8, v_g8, v_b8)
            v_r8 = __riscv_vnclipu_wx_u8m1(v_ri16, 0, vl);
            v_g8 = __riscv_vnclipu_wx_u8m1(v_gi16, 0, vl);
            v_b8 = __riscv_vnclipu_wx_u8m1(v_bi16, 0, vl);

            // 6. Store Strided (Re-interleave manually)
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