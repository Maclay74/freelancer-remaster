add_library(system MODULE
  "system.cpp"
  "system.rc"
)
target_link_libraries(system PRIVATE fl_build_options)
target_include_directories(system PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/system/../include")
