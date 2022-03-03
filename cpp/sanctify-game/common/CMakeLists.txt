add_proto_lib(
  TARGET_NAME sanctify_net_proto
  OUTDIR "sanctify-game-common/proto"
  PROTO_INFILES "${PROJECT_SOURCE_DIR}/../proto/sanctify-net.proto"
)

set (header_list
  "include/sanctify-game-common/gameplay/locomotion.h"
  "include/sanctify-game-common/gameplay/locomotion_components.h"
  "include/sanctify-game-common/gameplay/net_sync_components.h"
  "include/sanctify-game-common/gameplay/player_definition_components.h"
  "include/sanctify-game-common/net/entt_snapshot_translator.h"
  "include/sanctify-game-common/net/game_snapshot.h"
  "include/sanctify-game-common/net/net_config.h"
  "include/sanctify-game-common/net/reliable.h")

set (src_list
  "src/gameplay/locomotion.cc"
  "src/gameplay/locomotion_components.cc"
  "src/gameplay/net_sync_components.cc"
  "src/net/entt_snapshot_translator.cc"
  "src/net/game_snapshot.cc"
  "src/net/net_config.cc"
  "src/net/reliable.cc")

add_library(sanctify-game-common STATIC ${header_list} ${src_list})

target_include_directories(sanctify-game-common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(sanctify-game-common PUBLIC igcore glm sanctify_net_proto EnTT::EnTT)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify_net_proto AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME sanctify-game-common AS_LIB 1)
endif ()

if (IG_BUILD_TESTS)
  set(TEST_SRC_LIST
    "test/gameplay/locomotion_test.cc"
    "test/net/game_snapshot_test.cc")
  
  add_executable(sanctify-game-common_test ${TEST_SRC_LIST})
  target_link_libraries(sanctify-game-common_test gtest gtest_main sanctify-game-common)
  gtest_discover_tests(sanctify-game-common_test
    # Set a working directory both for GTest and Visual Studio to be happy
    WORKING_DIRECTORY ${PROJECT_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
  )

  set_property(TARGET sanctify-game-common_test PROPERTY CXX_STANDARD 17)
  target_compile_features(sanctify-game-common_test PUBLIC cxx_std_17)
endif ()