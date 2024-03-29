
INCLUDE(CheckIncludeFiles)

set (LIBCOLORTWIST_SOURCES  colortwist.h
                            colortwist.cpp
                            colortwist_c.h
                            colortwist_c.cpp
                            colortwist_ipp.h
                            colortwist_ipp.cpp
                            colortwist_avx.h
                            colortwist_avx.cpp
                            colortwist_sse.h
                            colortwist_sse.cpp
                            colortwist_neon.h
                            colortwist_neon.cpp
                            utils.h
                            utils.cpp)

add_library(libcolortwist STATIC
                ${LIBCOLORTWIST_SOURCES})

if(IPP_FOUND)
 set(COLORTWIST_HAS_IPP 1)
 target_include_directories(libcolortwist PRIVATE ${IPP_INCLUDE_DIRS})
else()
 set(COLORTWIST_HAS_IPP 0)
endif()

# check whether we can use AVX2-instructions -> we check for the presence of the file
CHECK_INCLUDE_FILES(immintrin.h IMMINTRINFOUND)
if (IMMINTRINFOUND)
 set(COLORTWIST_HAS_IMMINTRIN 1)
else()
 set(COLORTWIST_HAS_IMMINTRIN 0)
endif()

if (COLORTWIST_HAS_IMMINTRIN )
  include(CheckCXXSourceRuns)

  # Save the original value of CMAKE_REQUIRED_FLAGS
  set(ORIGINAL_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  # Add the -march=native flag
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -march=native")

  check_cxx_source_runs(
  "#include <immintrin.h>
  int main(void) { unsigned short u=1234; _mm_loadu_si16(&u); return 0; }"
  HAS_MM_LOADU_SI16_INTRINSIC)
  if (HAS_MM_LOADU_SI16_INTRINSIC)
    set(HAS_MM_LOADU_SI16_INTRINSIC 1)
  else()
    set(HAS_MM_LOADU_SI16_INTRINSIC 0)
  endif()

  check_cxx_source_runs(
  "#include <immintrin.h>
  int main(void) { unsigned int u=12345678; _mm_loadu_si32(&u); return 0; }"
  HAS_MM_LOADU_SI32_INTRINSIC)
  if (HAS_MM_LOADU_SI32_INTRINSIC)
    set(HAS_MM_LOADU_SI32_INTRINSIC 1)
  else()
    set(HAS_MM_LOADU_SI32_INTRINSIC 0)
  endif()

  check_cxx_source_runs(
  "#include <immintrin.h>
  int main(void) { unsigned int u=0;  __m128i mmreg = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15); _mm_storeu_si32(&u, mmreg); return 0; }"
  HAS_MM_STOREU_SI32_INTRINSIC)
  if (HAS_MM_STOREU_SI32_INTRINSIC)
    set(HAS_MM_STOREU_SI32_INTRINSIC 1)
  else()
    set(HAS_MM_STOREU_SI32_INTRINSIC 0)
  endif()

  # Restore the original CMAKE_REQUIRED_FLAGS
  set(CMAKE_REQUIRED_FLAGS "${ORIGINAL_CMAKE_REQUIRED_FLAGS}")
else()
  set(HAS_MM_LOADU_SI16_INTRINSIC 0)
  set(HAS_MM_LOADU_SI32_INTRINSIC 0)
  set(HAS_MM_STOREU_SI32_INTRINSIC 0)
endif()

# check whether we can use ARM-Neon-instructions -> we check for the presence of the file
CHECK_INCLUDE_FILES(arm_neon.h ARMNEONFOUND)
if (ARMNEONFOUND)
 if (MSVC)
  set(COMPILEDEFINITIONSTATICRUNTIMEVS "/MT")
  set(COMPILEDEFINITIONS_FOR_NEON "")
 else()
  set(COMPILEDEFINITIONS_FOR_NEON "-mfpu=neon")
 endif(MSVC)
 try_run(
   __RESULT_NEON_RUNTEST __RESULT_NEON_COMPILETEST
   ${CMAKE_CURRENT_BINARY_DIR}  
   ${CMAKE_CURRENT_SOURCE_DIR}/../cmakemodules/checks/cpu_neon.cpp 
   COMPILE_DEFINITIONS "${COMPILEDEFINITIONS_FOR_NEON} ${COMPILEDEFINITIONSTATICRUNTIMEVS}"
   OUTPUT_VARIABLE OUTPUT_NEONTESTCOMPILE)
   #message(STATUS "try_run: " ${OUTPUT_NEONTESTCOMPILE} : ${__RESULT_NEON_RUNTEST} - ${__RESULT_NEON_COMPILETEST})
 if (__RESULT_NEON_COMPILETEST AND __RESULT_NEON_RUNTEST EQUAL 0)
  message(STATUS "Test for Neon-support succeeded, Neon-support is enabled.")
  set(COLORTWIST_HAS_NEON 1)
 else()
   # try one more time, this time without any compile-definitions tweaks (with aarch64 it seems we must not set anything)  
   try_run(
     __RESULT_NEON_RUNTEST __RESULT_NEON_COMPILETEST
     ${CMAKE_CURRENT_BINARY_DIR}  
     ${CMAKE_CURRENT_SOURCE_DIR}/../cmakemodules/checks/cpu_neon.cpp 
     COMPILE_DEFINITIONS ${COMPILEDEFINITIONSTATICRUNTIMEVS}
     OUTPUT_VARIABLE OUTPUT_NEONTESTCOMPILE)

     if (__RESULT_NEON_COMPILETEST AND __RESULT_NEON_RUNTEST EQUAL 0)
      message(STATUS "Test for Neon-support succeeded, Neon-support is enabled.")
      set(COLORTWIST_HAS_NEON 1)
      set(COMPILEDEFINITIONS_FOR_NEON "")
     else()
      message(STATUS "Test for Neon-support failed, Neon-support is disabled.")
      set(COLORTWIST_HAS_NEON 0)
    endif()

 endif()
else()
 set(COLORTWIST_HAS_NEON 0)
endif()

# There seems to be a problem that the instrinsic "vcvtnq_u32_f32" was forgotten in early versions of GCC
# -> https://patchwork.ozlabs.org/project/gcc/patch/1601891882-13015-1-git-send-email-christophe.lyon@linaro.org/
# So, what we do here is to check whether this intrinsic is available. We have a fallback implementation if it is
# not available in place.
set(COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32 0)
if (COLORTWIST_HAS_NEON)
   try_run(
     __RESULT_NEON_VCVTNQ_U32_F32RUNTEST __RESULT_NEON_VCVTNQ_U32_F32COMPILETEST
     ${CMAKE_CURRENT_BINARY_DIR}  
     ${CMAKE_CURRENT_SOURCE_DIR}/../cmakemodules/checks/canuse_vcvtnq_u32_f32_intrinsic.cpp 
     COMPILE_DEFINITIONS "${COMPILEDEFINITIONS_FOR_NEON} ${COMPILEDEFINITIONSTATICRUNTIMEVS}" 
     COMPILE_OUTPUT_VARIABLE RESULT_NEONVCVTNQ_COMPILETEST
     RUN_OUTPUT_VARIABLE RESULT_NEONVCVTNQ_RUNTEST)
     #message(STATUS "vcvtnq_u32_f32-try_run: " ${RESULT_NEONVCVTNQ_COMPILETEST} - ${RESULT_NEONVCVTNQ_RUNTEST} )
     if (__RESULT_NEON_VCVTNQ_U32_F32COMPILETEST AND __RESULT_NEON_VCVTNQ_U32_F32RUNTEST EQUAL 0)
       message(STATUS "test whether intrinsic 'vcvtnq_u32_f32' is available -> YES")
       set(COLORTWISTLIB_CANUSENEONINTRINSIC_VCVTNQ_U32_F32 1)
     else()
       message(STATUS "test whether intrinsic 'vcvtnq_u32_f32' is available -> NO")
     endif()
endif()


set(COLORTWIST_HAS_SYS_AUXV_H 0)
if (ARMNEONFOUND)
 CHECK_INCLUDE_FILES("sys/auxv.h" SYSAUXHFOUND)
 if (SYSAUXHFOUND)
  set(COLORTWIST_HAS_SYS_AUXV_H 1)
 endif()
endif()


configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/colortwist_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/colortwist_config.h"
	ESCAPE_QUOTES @ONLY)                

#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (libcolortwist PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# this is necessary so that we can find "colortwist_config.h" which we created above
target_include_directories(libcolortwist PRIVATE  ${CMAKE_CURRENT_BINARY_DIR})  

message(STATUS "HOST-ARCHITECTURE: " ${CMAKE_SYSTEM_PROCESSOR})

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "amd64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64") 
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   set_source_files_properties(colortwist_avx.cpp PROPERTIES COMPILE_FLAGS /arch:AVX2)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   set_source_files_properties(colortwist_avx.cpp PROPERTIES COMPILE_FLAGS "-mfma -mavx2")
   set_source_files_properties(colortwist_sse.cpp PROPERTIES COMPILE_FLAGS "-msse4.1")
  endif()
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i686" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_32")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   set_source_files_properties(colortwist_avx.cpp PROPERTIES COMPILE_FLAGS /arch:AVX2)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   set_source_files_properties(colortwist_avx.cpp PROPERTIES COMPILE_FLAGS "-mfma -mavx2")
   set_source_files_properties(colortwist_sse.cpp PROPERTIES COMPILE_FLAGS "-msse4.1")
  endif()
endif()

# With ARM the situation seems to be: with aarch64 neon-support is guaranteed, and the compiler does not 
#  support the "fpu"-flag in this case
if (COLORTWIST_HAS_NEON)
 ##if (NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
 if (COMPILEDEFINITIONS_FOR_NEON)
  set_target_properties(libcolortwist PROPERTIES COMPILE_FLAGS ${COMPILEDEFINITIONS_FOR_NEON} )
  #message(STATUS "########################### ${COMPILEDEFINITIONS_FOR_NEON}")
 endif()
endif()

