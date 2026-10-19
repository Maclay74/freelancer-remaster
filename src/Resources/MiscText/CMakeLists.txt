add_library(MiscText MODULE
  "MiscText.rc"
)
set_target_properties(MiscText PROPERTIES LINKER_LANGUAGE CXX)
target_link_options(MiscText PRIVATE /NOENTRY /NODEFAULTLIB)
target_include_directories(MiscText PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(MiscText PROPERTIES OUTPUT_NAME "MiscText")
