add_library(imeui SHARED
  "CCaret.cpp"
  "CImeUiFont.cpp"
  "CInputBox.cpp"
  "IMEInputBox.cpp"
  "IMEFocus.cpp"
  "ImeUiCore.cpp"
  "ImeUiToolbar.cpp"
  "imeui_dllmain.cpp"
  "imeui.rc"
)
target_link_libraries(imeui PRIVATE fl_build_options)
target_include_directories(imeui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(imeui PRIVATE IMEUI_EXPORTS)
set_target_properties(imeui PROPERTIES OUTPUT_NAME "imeui")
