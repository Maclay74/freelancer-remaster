add_library(luaprofile MODULE
  "luaprofile.cpp"
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
  "lua32/src/lparser.c"
  "lua32/src/lstate.c"
  "lua32/src/lstring.c"
  "lua32/src/ltable.c"
  "lua32/src/ltm.c"
  "lua32/src/lundump.c"
  "lua32/src/lvm.c"
  "lua32/src/lzio.c"
  "lua32/lua32_layout.c"
  "luaprofile.rc"
)
target_link_libraries(luaprofile PRIVATE fl_build_options)
target_include_directories(luaprofile PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/luaprofile/../include" "${PROJECT_SOURCE_DIR}/src/luaprofile/lua32/include" "${PROJECT_SOURCE_DIR}/src/luaprofile/lua32/src")
set_target_properties(luaprofile PROPERTIES OUTPUT_NAME "luaprofile")
set_source_files_properties("lua32/src/lapi.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lauxlib.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lbuffer.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lbuiltin.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/ldo.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lfunc.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lgc.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/llex.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lmem.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lobject.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lparser.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lstate.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lstring.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/ltable.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/ltm.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lundump.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lvm.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/src/lzio.c" PROPERTIES LANGUAGE C)
set_source_files_properties("lua32/lua32_layout.c" PROPERTIES LANGUAGE C)
