add_library(Thorn MODULE
  "ThornModernAdapters.cpp"
  "Thorn.cpp"
  "ThornScene.cpp"
  "ThornControlCalls.cpp"
  "ParamCurve.cpp"
  "SplinePath.cpp"
  "ThornLua.cpp"
  "lua32/src/lapi.c"
  "lua32/src/lauxlib.c"
  "lua32/src/lbuffer.c"
  "lua32/src/lbuiltin.c"
  "lua32/src/ldo.c"
  "lua32/src/lfunc.c"
  "lua32/src/lgc.c"
  "lua32/src/llex.c"
  "lua32/src/lmem.c"
  "lua32/src/lobject.c"
  "lua32/src/lstate.c"
  "lua32/src/lstring.c"
  "lua32/src/ltable.c"
  "lua32/src/ltm.c"
  "lua32/src/lundump.c"
  "lua32/src/lvm.c"
  "lua32/src/lzio.c"
  "lua32/src/lib/lmathlib.c"
  "lua32/src/lib/liolib.c"
  "lua32/src/lib/lstrlib.c"
  "lua32/lparser_stub.c"
  "Thorn.rc"
)
target_link_libraries(Thorn PRIVATE fl_build_options)
target_include_directories(Thorn PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(Thorn PRIVATE lua32/include lua32/src)
set_target_properties(Thorn PROPERTIES OUTPUT_NAME "Thorn")
set_property(SOURCE "ThornLua.cpp" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include")
set_property(SOURCE "lua32/src/lapi.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lapi.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lauxlib.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lauxlib.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lbuffer.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lbuffer.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lbuiltin.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lbuiltin.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/ldo.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/ldo.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lfunc.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lfunc.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lgc.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lgc.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/llex.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/llex.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lmem.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lmem.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lobject.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lobject.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lstate.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lstate.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lstring.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lstring.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/ltable.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/ltable.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/ltm.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/ltm.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lundump.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lundump.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lvm.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lvm.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lzio.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lzio.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lib/lmathlib.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lib/lmathlib.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lib/liolib.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lib/liolib.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/src/lib/lstrlib.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/src/lib/lstrlib.c" PROPERTIES LANGUAGE C)
set_property(SOURCE "lua32/lparser_stub.c" APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/include" "${PROJECT_SOURCE_DIR}/src/Thorn/lua32/src")
set_source_files_properties("lua32/lparser_stub.c" PROPERTIES LANGUAGE C)
# Match the existing raw scalar REVERB argument forwarding configuration.
set_property(SOURCE ThornControlCalls.cpp APPEND PROPERTY COMPILE_OPTIONS /O2 /arch:SSE2)
