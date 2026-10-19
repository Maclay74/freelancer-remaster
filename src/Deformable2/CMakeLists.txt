add_library(Deformable2 MODULE
  "Deformable2.cpp"
  "EyeMaterial.cpp"
  "Deformable2.rc"
)
target_link_libraries(Deformable2 PRIVATE fl_build_options)
target_include_directories(Deformable2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(Deformable2 PROPERTIES OUTPUT_NAME "Deformable2")
