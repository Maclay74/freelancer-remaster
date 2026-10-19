add_library(serverresources MODULE
  "serverresources.rc"
)
set_target_properties(serverresources PROPERTIES LINKER_LANGUAGE CXX)
target_link_options(serverresources PRIVATE /NOENTRY /NODEFAULTLIB)
target_include_directories(serverresources PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(serverresources PROPERTIES OUTPUT_NAME "serverresources")
