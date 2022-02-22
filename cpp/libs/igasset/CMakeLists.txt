add_proto_lib(
  TARGET_NAME igasset_proto
  OUTDIR "igasset/proto"
  PROTO_INFILES "${PROJECT_SOURCE_DIR}/../proto/igasset.proto")

set (HEADER_LIST
  "include/igasset/draco_decoder.h"
  "include/igasset/draco_encoder.h"
  "include/igasset/igpack_loader.h"
  "include/igasset/image_data.h"
  "include/igasset/vertex_formats.h")

set (SRC_LIST
  "src/draco_decoder.cc"
  "src/draco_encoder.cc"
  "src/image_data.cc"
  "src/igpack_loader.cc")

set (VISUAL_STUDIO_EMPTY_SOURCES "src/vertex_formats.cc")

if (MSVC)
  list(APPEND SRC_LIST ${VISUAL_STUDIO_EMPTY_SOURCES})
endif ()

add_library(igasset STATIC ${HEADER_LIST} ${SRC_LIST})

# This weird target_include_directories is evidence I'm probably loading
#  Draco incorrectly in the linking section below...
target_include_directories(igasset PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_BINARY_DIR}"
  "${draco_SOURCE_DIR}/src")

target_link_libraries(igasset PUBLIC
    igasset_proto igcore igasync draco_lib igplatform stb ignav)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME igasset AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME igasset_proto AS_LIB 1)
endif ()