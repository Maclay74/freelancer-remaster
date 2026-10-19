add_library(RendComp MODULE
  "RendComp.cpp"
  "RenderManager.cpp"
  "Plugins.cpp"
  "TriMesh.cpp"
  "VMeshLibrary.cpp"
  "VMeshWire.cpp"
  "VMeshPart.cpp"
  "RendComp.rc"
)
target_link_libraries(RendComp PRIVATE fl_build_options)
target_include_directories(RendComp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/RendComp/../include")
set_target_properties(RendComp PROPERTIES OUTPUT_NAME "RendComp")
