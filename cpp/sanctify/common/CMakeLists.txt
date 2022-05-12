add_proto_lib_2(
  TARGET_NAME sanctify_common_proto
  PROTO_INFILES "${PROJECT_SOURCE_DIR}/sanctify/common/proto/primitives.proto"
                "${PROJECT_SOURCE_DIR}/sanctify/common/proto/common_logic_snapshot.proto"
)

add_subdirectory(util)
add_subdirectory(logic)
add_subdirectory(simple_client_app)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(user_input)
