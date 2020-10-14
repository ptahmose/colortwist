cmake_minimum_required(VERSION 3.0.0)
project(libcolortwist VERSION 0.1.0)

#include(CTest)
#enable_testing()

add_library(libcolortwist 
                colortwist.h
                colortwist.cpp)

#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (libcolortwist PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})