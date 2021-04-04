cmake_minimum_required (VERSION 3.1)
project (COLORTWIST)

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmakemodules" ${CMAKE_MODULE_PATH})

FIND_PACKAGE (IPP REQUIRED )

add_subdirectory (lib)
add_subdirectory (test)