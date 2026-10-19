add_executable(MemoryDiagnostics WIN32
  "main.cpp"
  "diagnostics.cpp"
)
target_link_libraries(MemoryDiagnostics PRIVATE fl_build_options)
target_include_directories(MemoryDiagnostics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(MemoryDiagnostics PRIVATE UNICODE _UNICODE)
target_compile_options(MemoryDiagnostics PRIVATE /utf-8 /U_MBCS)
