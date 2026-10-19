add_library(RemoteClient SHARED
  "RemoteClient_main.cpp"
  "ClientImpl.cpp"
  "rpc_server_channel_senders.cpp"
  "RemoteClient_transport.cpp"
  "rpc_client_packets.cpp"
  "../RemoteCommon/remote_packet_codecs.cpp"
  "../RemoteCommon/remote_server_channel_codecs.cpp"
)
target_link_libraries(RemoteClient PRIVATE fl_build_options)
target_include_directories(RemoteClient PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/RemoteClient/../include")
target_compile_definitions(RemoteClient PRIVATE FL_SERVER REMOTECLIENT_EXPORTS _WINDOWS _USRDLL)
