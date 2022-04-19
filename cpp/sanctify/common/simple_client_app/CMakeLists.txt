set(header_list
  "simple_client_app_base.h")

set(common_src_list
  "simple_client_app_base.cc")

if (EMSCRIPTEN)
  set(platform_src_list
    "evil_emscripten_hacks.cc"
    "evil_emscripten_hacks.h"
    "simple_client_app_base_web.cc")
else ()
  set(platform_src_list
    "simple_client_app_base_native.cc")
endif ()

add_library(sanctify-simple-client-app ${header_list} ${common_src_list} ${platform_src_list})
target_link_libraries(sanctify-simple-client-app PUBLIC igasync igplatform igcore iggpu)
target_include_directories(sanctify-simple-client-app PUBLIC "${SANCTIFY_INCLUDE_ROOT}")

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-simple-client-app AS_LIB 1)
else ()
  target_link_libraries(sanctify-simple-client-app PUBLIC
                        dawncpp glfw dawn_native dawn_wire dawn_utils dawn_proc dawn_common)
endif ()
