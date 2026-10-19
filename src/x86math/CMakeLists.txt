add_library(x86math MODULE
  "x86math.cpp"
  "dacom_stricmp.cpp"
  "x86math.rc"
)
target_link_libraries(x86math PRIVATE fl_build_options)
target_include_directories(x86math PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
