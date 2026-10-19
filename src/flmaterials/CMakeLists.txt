add_library(flmaterials MODULE
  "flmaterials.cpp"
  "MaterialImpl.cpp"
  "FLAppearance.cpp"
  "flmaterials.rc"
)
target_link_libraries(flmaterials PRIVATE fl_build_options)
target_include_directories(flmaterials PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/flmaterials/../include")
