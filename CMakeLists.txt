cmake_minimum_required (VERSION 3.15)

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0091 NEW)


project (COLORTWIST)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmakemodules" ${CMAKE_MODULE_PATH})

FIND_PACKAGE (IPP REQUIRED )

set(CMAKE_CXX_STANDARD 11)

enable_testing()

add_subdirectory (lib)
add_subdirectory (unittests)
add_subdirectory (test)