add_library(dacom SHARED
  "dacom.cpp"
  "dacom.rc"
  "dacom.def"
)
target_link_libraries(dacom PRIVATE fl_build_options)
target_include_directories(dacom PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
