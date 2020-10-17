cmake_minimum_required(VERSION 3.0.0)
project(libcolortwist VERSION 0.1.0)

#include(CTest)
#enable_testing()

#FIND_PACKAGE (IPP REQUIRED )
set (LIBCOLORTWIST_SOURCES  colortwist.h
                            colortwist.cpp
                            colortwist_ipp.h
                            colortwist_ipp.cpp
                            colortwist_avx.h
                            colortwist_avx.cpp)

#[[
add_library(libcolortwist 
                colortwist.h
                colortwist.cpp
                colortwist_ipp.cpp
                colortwist_avx.cpp)
]]                

add_library(libcolortwist 
                ${LIBCOLORTWIST_SOURCES})

if(IPP_FOUND)
 set(COLORTWIST_HAS_IPP 1)
 target_include_directories(libcolortwist PRIVATE ${IPP_INCLUDE_DIRS})
else()
 set(COLORTWIST_HAS_IPP 0)
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

message(STATUS ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64") 
  if (MSVC)
   set_source_files_properties(colortwist_avx.cpp PROPERTIES COMPILE_FLAGS /arch:AVX2)
  endif()
  if (CMAKE_COMPILER_IS_GNUCXX)
   set_source_files_properties(colortwist_avx.cpp PROPERTIES COMPILE_FLAGS -mfma)
  endif()
endif()