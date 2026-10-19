add_library(RP8 MODULE
  "RP8.cpp"
  "Mgsdb.cpp"
  "RP8.rc"
)
target_link_libraries(RP8 PRIVATE fl_build_options)
target_include_directories(RP8 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/RP8/../include")
set_target_properties(RP8 PROPERTIES OUTPUT_NAME "RP8")
