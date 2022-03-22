if (EMSCRIPTEN OR NOT IG_BUILD_SERVER)
  message(STATUS "Sanctify game server will not be generated for EMSCRIPTEN build")
  return()
endif ()

find_package(Boost 1.67 REQUIRED COMPONENTS system thread)

#
# Sources (most often modified, keep near top)
#
set(header_list
  "app/pve_game_server/ecs/net_state_update_system.h"
  "app/pve_game_server/ecs/player_connect_state.h"
  "app/pve_game_server/ecs/send_client_messages_system.h"
  "app/pve_game_server/net_event_organizer.h"
  "app/pve_game_server/pve_game_server.h"
  "app/ecs/player_nav_system.h"
  "app/systems/locomotion.h"
  "app/systems/net_serialize.h"
  "app/game_server.h"
  "app/net_events.h"
  "app/net_components.h"
  "net/build_game_token_exchanger.h"
  "net/dummy_game_token_exchanger.h"
  "net/igametokenexchanger.h"
  "net/net_server.h"
  "net/ws_server.h"
  "util/cli.h"
  "util/concurrentqueue.h"
  "util/event_scheduler.h"
  "util/server_clock.h"
  "util/types.h"
  "util/visit.h")

set(src_list
  "app/pve_game_server/ecs/net_state_update_system.cc"
  "app/pve_game_server/ecs/send_client_messages_system.cc"
  "app/pve_game_server/net_event_organizer.cc"
  "app/pve_game_server/pve_game_server.cc"
  "app/ecs/player_nav_system.cc"
  "app/systems/locomotion.cc"
  "app/systems/net_serialize.cc"
  "app/game_server.cc"
  "app/net_events.cc"
  "net/build_game_token_exchanger.cc"
  "net/dummy_game_token_exchanger.cc"
  "net/igametokenexchanger.cc"
  "net/net_server.cc"
  "net/ws_server.cc"
  "util/event_scheduler.cc"
  "util/server_clock.cc"
  "main.cc")

#
# Asset packs (usually shared with the client too)
#
set(asset_root "${PROJECT_SOURCE_DIR}/../assets")

build_ig_asset_pack_plan(
  TARGET_NAME terrain-navmesh-server-pack
  PLAN "${PROJECT_SOURCE_DIR}/sanctify-game/common/resources/terrain-navmesh.igpack-plan"
  INDIR "${asset_root}"
  INFILES
    "sanctify_arena/practice-arena.fbx"
  TARGET_OUTPUT_FILES
    "resources/terrain-navmesh.igpack"
)

build_ig_asset_pack_plan(
  TARGET_NAME pve-terrain-igpack-server
  PLAN "${PROJECT_SOURCE_DIR}/sanctify-game/common/resources/pve-terrain-navmesh.igpack-plan"
  INDIR "${asset_root}"
  INFILES
    "sanctify_arena/sanctify-pve.fbx"
  TARGET_OUTPUT_FILES
    "resources/terrain-pve.igpack"
)

add_executable(sanctify-game-server ${header_list} ${src_list})

# LEGAL NOTE - use libdatachannel and not libdatachannel-static. This is because
#  libdatachannel uses the LGPLv2 license and requires that a user be able to
#  freely link in their own build of the library in its place.
# Strictly speaking we could probably get around this a different way, but I'd
#  rather just avoid the headache entirely and link dynamically.
target_link_libraries(sanctify-game-server PUBLIC
  LibDataChannel::LibDataChannel CLI11 Boost::boost
  igcore igasync sanctify-game-common igasset)

target_include_directories(sanctify-game-server PRIVATE
  . "${websocketpp_SOURCE_DIR}")

if (MSVC)
  add_custom_command(TARGET sanctify-game-server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:datachannel>/datachannel${CMAKE_DEBUG_POSTFIX}.dll"
    $<TARGET_FILE_DIR:sanctify-game-server>
  )
endif ()

add_dependencies(sanctify-game-server terrain-navmesh-server-pack pve-terrain-igpack-server)
