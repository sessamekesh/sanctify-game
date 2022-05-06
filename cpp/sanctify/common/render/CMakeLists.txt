set (HEADER_LIST
  "common/camera_ubos.h"
  "common/pipeline_build_error.h"
  "common/render_components.h"
  "solid_static/ecs_util.h"
  "solid_static/instance_store.h"
  "solid_static/pipeline.h"
  "solid_static/solid_static_geo.h"
  "tonemap/ecs_util.h"
  "tonemap/pipeline.h"
  "viewport/update_arena_camera_system.h")

set (SRC_LIST
  "common/pipeline_build_error.cc"
  "solid_static/ecs_util.cc"
  "solid_static/instance_store.cc"
  "solid_static/pipeline.cc"
  "solid_static/solid_static_geo.cc"
  "tonemap/ecs_util.cc"
  "tonemap/pipeline.cc"
  "viewport/update_arena_camera_system.cc")

add_library(sanctify-common-render STATIC ${HEADER_LIST} ${SRC_LIST})
target_include_directories(sanctify-common-render PUBLIC "${SANCTIFY_INCLUDE_ROOT}")

target_link_libraries(sanctify-common-render PUBLIC
                      igcore iggpu glm igecs sanctify-common-util)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-common-render AS_LIB 1)
endif ()
