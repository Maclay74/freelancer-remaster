add_library(ReadFile MODULE
  "DacomStricmp.cpp"
  "ReadFile.cpp"
  "ReadFile.rc"
)
target_link_libraries(ReadFile PRIVATE fl_build_options)
target_include_directories(ReadFile PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
