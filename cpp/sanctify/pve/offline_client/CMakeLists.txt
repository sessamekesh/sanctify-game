set(hdr_list
  "offline_client_app.h")

set(src_list
  "offline_client_app.cc")

if (EMSCRIPTEN)
  set(platform_src_list
    "embind_main.cc")
else ()
  set(platform_src_list
    "native_main.cc")
endif () 

add_proto_lib(
  TARGET_NAME sanctify-pve-offline-client-config-proto
  OUTDIR "pve/offline_client/pb"
  PROTO_INFILES "pve_offline_client_config.proto"
)

add_executable(sanctify-pve-offline-client ${platform_src_list} ${hdr_list} ${src_list})
target_link_libraries(sanctify-pve-offline-client PUBLIC
                      sanctify-simple-client-app sanctify-common-logic
                      sanctify-common-scene sanctify-pve-offline-client-config-proto)
target_include_directories(sanctify-simple-client-app PUBLIC "${SANCTIFY_INCLUDE_ROOT}")

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-pve-offline-client
                             EXPORT_NAME SanctifyPveOfflineClient)
endif ()
