add_library(RP9 MODULE
  "RP9.cpp"
  "RP9Ex.cpp"
  "Mgsdb.cpp"
  "RP9.rc"
)
target_link_libraries(RP9 PRIVATE fl_build_options)
target_include_directories(RP9 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/RP9/../include")
target_compile_definitions(RP9 PRIVATE RP_DIRECT3D9)
set_target_properties(RP9 PROPERTIES OUTPUT_NAME "RP9")
