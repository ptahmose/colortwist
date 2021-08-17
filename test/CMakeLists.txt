add_executable (testcolortwist testcolortwist.cpp)

target_link_libraries (testcolortwist LINK_PUBLIC libcolortwist)

#set_target_properties(testcolortwist PROPERTIES VS_GLOBAL_UseIntelIPP "Sequential") # Parallel_Static

#message(STATUS "IPP-Libraries: ${IPP_LIBRARIES}")

if(IPP_FOUND)
 target_link_libraries (testcolortwist LINK_PUBLIC "${IPP_LIB_SEARCHPATH}/ippccmt.lib" "${IPP_LIB_SEARCHPATH}/ippcoremt.lib")
 # Linux/OSX would be "libipp<domain>.a" -> libippcc.a
endif()