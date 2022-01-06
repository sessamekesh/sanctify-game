add_proto_lib(
  TARGET_NAME igasset_proto
  OUTDIR "igasset/proto"
  PROTO_INFILES "${PROJECT_SOURCE_DIR}/../proto/igasset.proto")

set (HEADER_LIST
  "include/igasset/draco_decoder.h"
  "include/igasset/draco_encoder.h"
  "include/igasset/heightmap.h"
  "include/igasset/igpack_loader.h"
  "include/igasset/image_data.h"
  "include/igasset/vertex_formats.h")

set (SRC_LIST
  "src/draco_decoder.cc"
  "src/draco_encoder.cc"
  "src/heightmap.cc"
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
  "${PROJECT_SOURCE_DIR}/extern/draco/src")

target_link_libraries(igasset PUBLIC igasset_proto igcore igasync draco_lib igplatform stb)

set_property(TARGET igasset PROPERTY CXX_STANDARD 17)
target_compile_features(igasset PUBLIC cxx_std_17)

if (IG_BUILD_TESTS)
  set(TEST_SRC_LIST
    "test/heightmap_test.cc")
  
  add_executable(igasset_test ${TEST_SRC_LIST})
  target_link_libraries(igasset_test gtest gtest_main igasset)
  gtest_discover_tests(igasset_test
    WORKING_DIRECTORY ${PROJECT_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}")
  set_property(TARGET igasset_test PROPERTY CXX_STANDARD 17)
  target_compile_features(igasset_test PUBLIC cxx_std_17)
endif ()