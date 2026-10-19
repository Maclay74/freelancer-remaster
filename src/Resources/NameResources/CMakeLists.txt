add_library(NameResources MODULE
  "NameResources.rc"
)
set_target_properties(NameResources PROPERTIES LINKER_LANGUAGE CXX)
target_link_options(NameResources PRIVATE /NOENTRY /NODEFAULTLIB)
target_include_directories(NameResources PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(NameResources PROPERTIES OUTPUT_NAME "NameResources")
