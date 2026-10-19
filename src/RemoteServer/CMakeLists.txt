add_library(RemoteServer SHARED
  "RemoteServer_main.cpp"
  "rpc_server_channel_receive.cpp"
  "IServerImpl.cpp"
  "rpc_server_packets.cpp"
  "../RemoteCommon/remote_packet_codecs.cpp"
  "../RemoteCommon/remote_server_channel_codecs.cpp"
)
target_link_libraries(RemoteServer PRIVATE fl_build_options)
target_include_directories(RemoteServer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/RemoteServer/../include")
target_compile_definitions(RemoteServer PRIVATE REMOTESERVER_EXPORTS FL_CLIENT _WINDOWS _USRDLL)
set_target_properties(RemoteServer PROPERTIES OUTPUT_NAME "remoteserver")
