set(hdr_list
  "loading_util/common.h")

set(src_list
  "loading_util/common.cc")

add_library(sanctify-pve-render-common STATIC ${hdr_list} ${src_list})
target_include_directories(sanctify-pve-render-common PUBLIC "${SANCTIFY_INCLUDE_ROOT}")
target_link_libraries(sanctify-pve-render-common PUBLIC
           sanctify-common-render
           sanctify-simple-client-app)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-pve-render-common AS_LIB 1)
endif ()
