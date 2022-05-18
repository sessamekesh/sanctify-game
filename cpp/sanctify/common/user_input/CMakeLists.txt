set(hdr_list
  "ecs_util.h"
  "events.h")

set(src_list
  "ecs_util.cc")

add_library(sanctify-common-user-input STATIC ${hdr_list} ${src_list} ${platform_src})
target_include_directories(sanctify-common-user-input PUBLIC "${SANCTIFY_INCLUDE_DIR}")

target_link_libraries(sanctify-common-user-input PUBLIC igecs)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-common-user-input AS_LIB 1)
endif ()
