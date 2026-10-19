add_library(ximage MODULE
  "ximage.cpp"
  "ximage.rc"
)
target_link_libraries(ximage PRIVATE fl_build_options)
target_include_directories(ximage PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/ximage/../include")
set_target_properties(ximage PROPERTIES OUTPUT_NAME "ximage")
