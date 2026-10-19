add_library(EngBase MODULE
  "EngBase.cpp"
  "EngBaseMixer.cpp"
  "EngBase.rc"
)
target_link_libraries(EngBase PRIVATE fl_build_options)
target_include_directories(EngBase PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(EngBase PROPERTIES OUTPUT_NAME "EngBase")
