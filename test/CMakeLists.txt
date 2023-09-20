add_executable (testcolortwist testcolortwist.cpp)

target_link_libraries (testcolortwist LINK_PUBLIC libcolortwist)

#message(STATUS "IPP-Libraries: ${IPP_LIBRARIES}")

if(IPP_FOUND)
  if(WIN32)
   target_link_libraries (testcolortwist LINK_PUBLIC "${IPP_LIB_SEARCHPATH}/ippccmt.lib" "${IPP_LIB_SEARCHPATH}/ippcoremt.lib")
  elseif(UNIX)
    target_link_libraries (testcolortwist LINK_PUBLIC "${IPP_LIB_SEARCHPATH}/libippcc.a" "${IPP_LIB_SEARCHPATH}/libippcore.a")
  endif()
 # Linux/OSX would be "libipp<domain>.a" -> libippcc.a
endif()