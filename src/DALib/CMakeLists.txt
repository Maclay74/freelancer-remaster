add_library(DALib SHARED
  "DALib.cpp"
  "md5hash.cpp"
  "CDPMsgList.cpp"
  "CDPNetCodec.cpp"
  "CDPClient.cpp"
  "CDPClientProxy.cpp"
  "CDPServer.cpp"
  "CGunWrapper.cpp"
  "DALib.rc"
)
target_link_libraries(DALib PRIVATE fl_build_options)
target_include_directories(DALib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "../zlib/vendor")
target_compile_definitions(DALib PRIVATE DALIB_EXPORTS ZLIB_WINAPI)
