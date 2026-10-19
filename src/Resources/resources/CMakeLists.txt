add_library(resources MODULE
  "resources.rc"
)
set_target_properties(resources PROPERTIES LINKER_LANGUAGE CXX)
target_link_options(resources PRIVATE /NOENTRY /NODEFAULTLIB)
target_include_directories(resources PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(resources PROPERTIES OUTPUT_NAME "resources")
