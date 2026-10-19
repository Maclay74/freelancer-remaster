add_library(SoundManager MODULE
  "SoundManager.cpp"
  "SoundManager.rc"
)
target_link_libraries(SoundManager PRIVATE fl_build_options)
target_include_directories(SoundManager PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
