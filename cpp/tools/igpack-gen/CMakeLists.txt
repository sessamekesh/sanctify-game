add_proto_lib(
  TARGET_NAME igpack_plan_proto
  OUTDIR "igpack-gen/proto"
  PROTO_INFILES "${PROJECT_SOURCE_DIR}/../proto/igpack-plan.proto"
)

set(HEADER_LIST
  "converters/assimp_geo_processor.h"
  "converters/wgsl_processor.h"
  "util/assimp_scene_cache.h"
  "util/file_cache.h"
  "plan_executor.h")

set (SRC_LIST
  "converters/assimp_geo_processor.cc"
  "converters/wgsl_processor.cc"
  "util/assimp_scene_cache.cc"
  "util/file_cache.cc"
  "plan_executor.cc"

  "main.cc")

add_executable(igpack-gen ${SRC_LIST} ${HEADER_LIST})

target_link_libraries(igpack-gen PUBLIC
    igasset stb igasset_proto igpack_plan_proto draco_lib CLI11
    assimp)
target_include_directories(igpack-gen PRIVATE "${CMAKE_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/extern/draco/src" .)