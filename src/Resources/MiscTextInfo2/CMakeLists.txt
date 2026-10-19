add_library(MiscTextInfo2 MODULE
  "MiscTextInfo2.rc"
)
set_target_properties(MiscTextInfo2 PROPERTIES LINKER_LANGUAGE CXX)
target_link_options(MiscTextInfo2 PRIVATE /NOENTRY /NODEFAULTLIB)
target_include_directories(MiscTextInfo2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(MiscTextInfo2 PROPERTIES OUTPUT_NAME "MiscTextInfo2")
