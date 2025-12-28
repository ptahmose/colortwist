include(CheckCXXSourceCompiles)

# Function to check for RISC-V Vector Tuple Support
function(check_rvv_tuple_support OUT_VAR)
    # Save current flags
    set(CMAKE_REQUIRED_FLAGS "-march=rv64gcv -mabi=lp64d")
    
    check_cxx_source_compiles("
        #include <riscv_vector.h>
        #include <stdint.h>
        
        int main() {
            // Test 1: Can we instantiate a Tuple type?
            // (Only available in finalized v1.0 compilers like GCC 14)
            vuint8m1x3_t tuple;
            
            // Test 2: Can we access the tuple set/get intrinsics?
            size_t vl = 10;
            uint8_t val = 0;
            vuint8m1_t v_val = __riscv_vmv_v_x_u8m1(val, vl);
            
            // This intrinsic is specific to the Tuple standard
            tuple = __riscv_vset_v_u8m1x3_x0(tuple, v_val);
            
            return 0;
        }
    " ${OUT_VAR})
endfunction()