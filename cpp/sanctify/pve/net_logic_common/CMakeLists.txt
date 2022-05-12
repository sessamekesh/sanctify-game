set(hdr_list
  "client_message_queue.h")

set(src_list
  "client_message_queue.cc")

add_library(sanctify-pve-net-logic-common STATIC ${hdr_list} ${src_list})
target_include_directories(sanctify-pve-net-logic-common PUBLIC igecs)

add_proto_lib_2(
  TARGET_NAME sanctify_pve_net_logic_proto
  PROTO_INFILES "proto/client_messages.proto"
  PROTO_LIB_DEPS sanctify_common_proto
)

target_link_libraries(sanctify-pve-net-logic-common PUBLIC
  igecs sanctify_pve_net_logic_proto sanctify_common_proto)
target_include_directories(sanctify-pve-net-logic-common PUBLIC "${SANCTIFY_INCLUDE_ROOT}")

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-pve-net-logic-common AS_LIB 1)
endif ()
