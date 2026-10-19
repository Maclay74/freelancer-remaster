add_library(Server SHARED
  "IServerImpl.cpp"
  "server_pub.cpp"
  "server_classes.cpp"
  "server_chat.cpp"
  "server_gameobject.cpp"
  "server_observer.cpp"
  "server_createinfo.cpp"
  "server_savegame.cpp"
  "server_savefile.cpp"
  "server_fuse.cpp"
  "server_loot.cpp"
  "server_mine.cpp"
  "server_guided.cpp"
  "server_countermeasure.cpp"
  "server_basedb.cpp"
)
target_link_libraries(Server PRIVATE fl_build_options)
target_include_directories(Server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(Server PRIVATE _WINDOWS _USRDLL SERVER_EXPORTS)
