add_proto_lib(
  TARGET_NAME sanctify_common_proto
  OUTDIR "common/pb"
  PROTO_INFILES "${PROJECT_SOURCE_DIR}/sanctify/common/proto/primitives.proto"
                "${PROJECT_SOURCE_DIR}/sanctify/common/proto/common_logic_snapshot.proto"
  INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/sanctify/common/proto"
)

add_subdirectory(util)
add_subdirectory(logic)
add_subdirectory(simple_client_app)
add_subdirectory(render)
add_subdirectory(scene)
