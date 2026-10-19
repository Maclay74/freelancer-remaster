add_library(rpclocal SHARED
  "rpclocal.cpp"
  "rpc_transport.cpp"
  "rpc_packet_registry.cpp"
  "rpc_client_proxy.cpp"
  "rpc_dispatch.cpp"
  "rpc_payload_ch1.cpp"
  "rpc_payload_ch2.cpp"
  "rpc_payload_ch3.cpp"
  "rpc_containers.cpp"
  "rpc_wire_ch1.cpp"
  "rpc_wire_ch2.cpp"
  "rpc_wire_generic.cpp"
  "rpc_wire_fields.cpp"
  "rpc_wire_varlen.cpp"
  "rpc_wire_closeout.cpp"
  "rpc_wire_ch3.cpp"
  "rpc_packet_dump.cpp"
)
target_link_libraries(rpclocal PRIVATE fl_build_options)
target_include_directories(rpclocal PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/rpclocal/../include")
target_compile_definitions(rpclocal PRIVATE _WINDOWS _USRDLL RPCLOCAL_EXPORTS)
