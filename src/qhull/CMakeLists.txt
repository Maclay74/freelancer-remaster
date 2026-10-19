add_library(qhull STATIC
  "vendor/geom.c"
  "vendor/geom2.c"
  "vendor/global.c"
  "vendor/io.c"
  "vendor/mem.c"
  "vendor/merge.c"
  "vendor/poly.c"
  "vendor/poly2.c"
  "vendor/qhull.c"
  "vendor/qset.c"
  "vendor/stat.c"
  "vendor/user.c"
)
target_link_libraries(qhull PRIVATE fl_build_options)
target_include_directories(qhull PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/qhull/vendor")
target_compile_definitions(qhull PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
