add_library(movie SHARED
  "MediaPlayer.cpp"
  "MovieSha.cpp"
  "MovieMod.cpp"
  "MovieAce.cpp"
  "MovieExp.cpp"
  "MovieField.cpp"
  "MovieFieldHigh.cpp"
  "MovieFieldMid.cpp"
  "MovieGraph.cpp"
  "MovieRuntime.cpp"
  "MovieCca.cpp"
  "MovieWindow.cpp"
  "MovieD4.cpp"
  "MovieWide.cpp"
  "MoviePlay.cpp"
  "MovieObject.cpp"
)
target_link_libraries(movie PRIVATE fl_build_options)
target_include_directories(movie PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(movie PROPERTIES OUTPUT_NAME "movie")
