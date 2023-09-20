# SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY "https://github.com/google/googletest"
  GIT_TAG        v1.12.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(
  colortwist_unittests
  colortwist_tests.cpp
)

#target_link_libraries (colortwist_unittests LINK_PUBLIC libcolortwist)

target_link_libraries(
  colortwist_unittests
  LINK_PUBLIC
  libcolortwist
  gtest_main
)

if(IPP_FOUND)
  if(WIN32)
   target_link_libraries (colortwist_unittests LINK_PUBLIC "${IPP_LIB_SEARCHPATH}/ippccmt.lib" "${IPP_LIB_SEARCHPATH}/ippcoremt.lib")
  elseif(UNIX)
    target_link_libraries (colortwist_unittests LINK_PUBLIC "${IPP_LIB_SEARCHPATH}/libippcc.a" "${IPP_LIB_SEARCHPATH}/libippcore.a")
  endif()
 # Linux/OSX would be "libipp<domain>.a" -> libippcc.a
endif()

include(GoogleTest)
gtest_discover_tests(colortwist_unittests)