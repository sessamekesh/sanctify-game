set(header_list
  "preload/preload_scene.h"
  "preload/preload_shader.h"
  "scene_base.h")

set(src_list
  "preload/preload_scene.cc"
  "preload/preload_shader.cc")

add_library(sanctify-common-scene ${header_list} ${src_list})
target_link_libraries(sanctify-common-scene PUBLIC igasync igcore iggpu igecs)
target_include_directories(sanctify-common-scene PUBLIC "${SANCTIFY_INCLUDE_ROOT}")

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-common-scene AS_LIB 1)
endif ()
