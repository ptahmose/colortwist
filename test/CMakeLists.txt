# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx". The extensions are automatically found.
add_executable (testcolortwist testcolortwist.cpp)

# Link the executable to the Hello library. Since the Hello library has
# public include directories we will use those link directories when building
# helloDemo
target_link_libraries (testcolortwist LINK_PUBLIC libcolortwist)

#message(STATUS "IPP-Libraries: ${IPP_LIBRARIES}")

if(IPP_FOUND)
 target_link_libraries (testcolortwist LINK_PUBLIC "${IPP_LIB_SEARCHPATH}/ippccmt.lib" "${IPP_LIB_SEARCHPATH}/ippcoremt.lib")
 # Linux/OSX would be "libipp<domain>.a" -> libippcc.a
endif()