add_proto_lib(
  TARGET_NAME sanctify_net_proto
  OUTDIR "sanctify-game-common/proto"
  PROTO_INFILES "${PROJECT_SOURCE_DIR}/../proto/sanctify-net.proto"
)

set (header_list
  "include/sanctify-game-common/net/net_config.h"
  "include/sanctify-game-common/net/reliable.h")

set (src_list
  "src/net/net_config.cc"
  "src/net/reliable.cc")

add_library(sanctify-game-common STATIC ${header_list} ${src_list})

target_include_directories(sanctify-game-common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(sanctify-game-common PUBLIC igcore glm sanctify_net_proto)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify_net_proto AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME sanctify-game-common AS_LIB 1)
endif ()