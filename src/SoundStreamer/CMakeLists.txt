add_library(SoundStreamer MODULE
  "SoundStreamer.cpp"
  "SoundStreamer.rc"
)
target_link_libraries(SoundStreamer PRIVATE fl_build_options)
target_include_directories(SoundStreamer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
