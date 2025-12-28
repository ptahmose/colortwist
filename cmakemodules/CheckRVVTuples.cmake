include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# Function: check_rvv_vcreate_support
# Checks if the compiler supports RVV 1.0 'vcreate' intrinsic for tuples.
# This confirms we are on a modern GCC 14+ toolchain.
function(check_rvv_vcreate_support OUT_VAR)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "-march=rv64gcv -mabi=lp64d")
    
    check_cxx_source_compiles("
        #include <riscv_vector.h>
        #include <stdint.h>
        
        int main() {
            // 1. Create dummy vectors
            size_t vl = 10;
            vuint8m1_t v0 = __riscv_vmv_v_x_u8m1(0, vl);
            vuint8m1_t v1 = __riscv_vmv_v_x_u8m1(1, vl);
            vuint8m1_t v2 = __riscv_vmv_v_x_u8m1(2, vl);
            
            // 2. Test the 'vcreate' intrinsic (The GCC 14 way)
            // It constructs a tuple from N arguments.
            vuint8m1x3_t tuple = __riscv_vcreate_v_u8m1x3(v0, v1, v2);
            
            // 3. Test 'vget' (which your error log confirmed exists)
            vuint8m1_t extracted = __riscv_vget_v_u8m1x3_u8m1(tuple, 0);
            
            return 0;
        }
    " ${OUT_VAR})
    cmake_pop_check_state()
endfunction()