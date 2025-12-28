add_executable (testcolortwist testcolortwist.cpp)

target_link_libraries (testcolortwist LINK_PUBLIC libcolortwist)

if(IPP_FOUND)
  if(WIN32)
   target_link_libraries (testcolortwist LINK_PUBLIC "${IPP_LIB_SEARCHPATH}/ippccmt.lib" "${IPP_LIB_SEARCHPATH}/ippcoremt.lib")
  elseif(UNIX)
    target_link_libraries (testcolortwist LINK_PUBLIC "${IPP_LIB_SEARCHPATH}/libippcc.a" "${IPP_LIB_SEARCHPATH}/libippcore.a")
  endif()
endif()