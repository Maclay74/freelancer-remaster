add_library(Shading MODULE
  "Shading.cpp"
  "Material.cpp"
  "MaterialLibrary.cpp"
  "MaterialRegex.cpp"
  "MaterialBatcher.cpp"
  "TextureLibrary.cpp"
  "MaterialAnimation.cpp"
  "Shading.rc"
)
target_link_libraries(Shading PRIVATE fl_build_options)
target_include_directories(Shading PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(Shading PROPERTIES OUTPUT_NAME "Shading")
