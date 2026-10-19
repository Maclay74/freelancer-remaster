add_library(InfoCards MODULE
  "InfoCards.rc"
)
set_target_properties(InfoCards PROPERTIES LINKER_LANGUAGE CXX)
target_link_options(InfoCards PRIVATE /NOENTRY /NODEFAULTLIB)
target_include_directories(InfoCards PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(InfoCards PROPERTIES OUTPUT_NAME "InfoCards")
