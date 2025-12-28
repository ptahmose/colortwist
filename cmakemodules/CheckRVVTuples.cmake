include(CheckCXXSourceCompiles)

# Function: check_rvv_tuple_support
# Checks if the compiler supports RVV 1.0 Tuple types with index-based accessors.
# Output Variable: OUT_VAR (set to TRUE or FALSE)
function(check_rvv_tuple_support OUT_VAR)
    # Ensure we use the flags required for vector extensions
    set(CMAKE_REQUIRED_FLAGS "-march=rv64gcv -mabi=lp64d")
    
    check_cxx_source_compiles("
        #include <riscv_vector.h>
        #include <stdint.h>
        
        int main() {
            // 1. Declare a tuple type (Vector of 3 uint8_t registers)
            vuint8m1x3_t tuple;
            
            // 2. Create a value to insert
            size_t vl = 10;
            uint8_t val = 0;
            vuint8m1_t v_val = __riscv_vmv_v_x_u8m1(val, vl);
            
            // 3. Test the RVV 1.0 'vset' intrinsic
            // Syntax: __riscv_vset_v_{TupleType}_{ElementType}(tuple, index, value)
            // Notice we pass '0' as the index, instead of using '_x0' in the name.
            tuple = __riscv_vset_v_u8m1x3_u8m1(tuple, 0, v_val);
            
            // 4. Test the RVV 1.0 'vget' intrinsic
            // Syntax: __riscv_vget_v_{TupleType}_{ElementType}(tuple, index)
            vuint8m1_t v_extracted = __riscv_vget_v_u8m1x3_u8m1(tuple, 0);
            
            return 0;
        }
    " ${OUT_VAR})
endfunction()