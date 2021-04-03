cmake_minimum_required(VERSION 3.0.0)
project(libcolortwist VERSION 0.1.0)

INCLUDE(CheckIncludeFiles)

set (LIBCOLORTWIST_SOURCES  colortwist.h
                            colortwist.cpp
                            colortwist_c.h
                            colortwist_c.cpp
                            colortwist_ipp.h
                            colortwist_ipp.cpp
                            colortwist_avx.h
                            colortwist_avx.cpp
                            colortwist_neon.h
                            colortwist_neon.cpp
                            utils.h
                            utils.cpp)

add_library(libcolortwist 
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
 set(COLORTWISE_HAS_AVX 1)
else()
 set(COLORTWISE_HAS_AVX 0)
endif()

# check whether we can use ARM-Neon-instructions -> we check for the presence of the file
CHECK_INCLUDE_FILES(arm_neon.h ARMNEONFOUND)
if (ARMNEONFOUND)
 try_run(
   __RESULT_NEON_RUNTEST __RESULT_NEON_COMPILETEST
   ${CMAKE_CURRENT_BINARY_DIR}  
   ${CMAKE_CURRENT_SOURCE_DIR}/../cmakemodules/checks/cpu_neon.cpp 
   COMPILE_DEFINITIONS -mfpu=neon
   OUTPUT_VARIABLE OUTPUT_NEONTESTCOMPILE)
   #message(STATUS "try_run: " ${__RESULT_NEON_RUNTEST} - ${__RESULT_NEON_COMPILETEST})
 if (__RESULT_NEON_COMPILETEST AND __RESULT_NEON_RUNTEST EQUAL 0)
  message(STATUS "Test for Neon-support succeeded, Neon-support is enabled.")
  set(COLORTWIST_HAS_NEON 1)
  set(COMPILEDEFINITIONS_FOR_NEON "-mfpu=neon")
 else()
   # try one more time, this time without any compile-definitions tweaks (with aarch64 it seems we must not set anything)  
   try_run(
     __RESULT_NEON_RUNTEST __RESULT_NEON_COMPILETEST
     ${CMAKE_CURRENT_BINARY_DIR}  
     ${CMAKE_CURRENT_SOURCE_DIR}/../cmakemodules/checks/cpu_neon.cpp 
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
     COMPILE_DEFINITIONS "${COMPILEDEFINITIONS_FOR_NEON}" 
     COMPILE_OUTPUT_VARIABLE RESULT_NEONVCVTNQ_COMPILETEST
     RUN_OUTPUT_VARIABLE RESULT_NEONVCVTNQ_RUNTEST)
     message(STATUS "vcvtnq_u32_f32-try_run: " ${RESULT_NEONVCVTNQ_COMPILETEST} - ${RESULT_NEONVCVTNQ_RUNTEST} )
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

message(STATUS "ARCHITECTURE: " ${CMAKE_SYSTEM_PROCESSOR})

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64") 
  if (MSVC)
   set_source_files_properties(colortwist_avx.cpp PROPERTIES COMPILE_FLAGS /arch:AVX2)
  endif()
  if (CMAKE_COMPILER_IS_GNUCXX)
   set_source_files_properties(colortwist_avx.cpp PROPERTIES COMPILE_FLAGS "-mfma -mavx2")
  endif()
endif()

# With ARM the situation seems to be: with aarch64 neon-support is guaranteed, and the compiler does not 
#  support the "fpu"-flag in this case
if (COLORTWIST_HAS_NEON)
 ##if (NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
 if (COMPILEDEFINITIONS_FOR_NEON)
  set_target_properties(libcolortwist PROPERTIES COMPILE_FLAGS ${COMPILEDEFINITIONS_FOR_NEON} )
 endif()
endif()

