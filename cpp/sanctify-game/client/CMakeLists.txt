#
# Sources (most often modified, keep at top)
#
set(header_list
  "platform_src/netclient/game_api.h"
  "platform_src/netclient/net_client.h"
  "platform_src/netclient/ws_client_base.h"
  "platform_src/app_base.h"
  "src/app_startup_scene/app_startup_scene.h"
  "src/app_startup_scene/startup_shader_src.h"
  "src/game_scene/factory_util/build_character_geo.h"
  "src/game_scene/factory_util/build_debug_geo_shit.h"
  "src/game_scene/factory_util/build_net_client.h"
  "src/game_scene/factory_util/build_terrain_shit.h"
  "src/game_scene/systems/player_move_indicator_render_system.h"
  "src/game_scene/systems/player_render_system.h"
  "src/game_scene/net/reconcile_net_state_system.h"
  "src/game_scene/net/snapshot_cache.h"
  "src/game_scene/game_scene.h"
  "src/game_scene/game_scene_factory.h"
  "src/io/arena_camera_controller/arena_camera_input.h"
  "src/io/arena_camera_controller/arena_camera_mouse_input_listener.h"
  "src/io/viewport_click/viewport_click_controller_input.h"
  "src/io/viewport_click/viewport_click_mouse_impl.h"
  "src/io/glfw_event_emitter.h"
  "src/render/camera/arena_camera.h"
  "src/render/debug_geo/debug_geo.h"
  "src/render/debug_geo/debug_geo_pipeline.h"
  "src/render/solid_animated/solid_animated_geo.h"
  "src/render/solid_animated/solid_animated_pipeline.h"
  "src/render/terrain/terrain_geo.h"
  "src/render/terrain/terrain_pipeline.h"
  "src/util/camera_util.h"
  "src/sanctify_client_app.h")

set(src_list
  "platform_src/netclient/game_api.cc"
  "platform_src/netclient/net_client.cc"
  "platform_src/netclient/ws_client_base.cc"
  "src/game_scene/factory_util/build_character_geo.cc"
  "src/game_scene/factory_util/build_debug_geo_shit.cc"
  "src/game_scene/factory_util/build_net_client.cc"
  "src/game_scene/factory_util/build_terrain_shit.cc"
  "src/app_startup_scene/app_startup_scene.cc"
  "src/game_scene/systems/player_move_indicator_render_system.cc"
  "src/game_scene/systems/player_render_system.cc"
  "src/game_scene/net/reconcile_net_state_system.cc"
  "src/game_scene/net/snapshot_cache.cc"
  "src/game_scene/game_scene.cc"
  "src/game_scene/game_scene_factory.cc"
  "src/io/arena_camera_controller/arena_camera_mouse_input_listener.cc"
  "src/io/viewport_click/viewport_click_controller_input.cc"
  "src/io/viewport_click/viewport_click_mouse_impl.cc"
  "src/io/glfw_event_emitter.cc"
  "src/render/camera/arena_camera.cc"
  "src/render/debug_geo/debug_geo.cc"
  "src/render/debug_geo/debug_geo_pipeline.cc"
  "src/render/solid_animated/solid_animated_geo.cc"
  "src/render/solid_animated/solid_animated_pipeline.cc"
  "src/render/terrain/terrain_geo.cc"
  "src/render/terrain/terrain_pipeline.cc"
  "src/util/camera_util.cc"
  "src/sanctify_client_app.cc")

#
# Asset Packs
#
set(asset_root "${PROJECT_SOURCE_DIR}/../assets")

build_ig_asset_pack_plan(
  TARGET_NAME terrain-geo-igpack
  PLAN resources/terrain-geo.igpack-plan
  INDIR "${asset_root}"
  INFILES
    "sanctify_arena/practice-arena.fbx"
    "engine/solid_3d.vert.wgsl"
    "engine/solid_3d.frag.wgsl"
  TARGET_OUTPUT_FILES
    "resources/terrain-geo.igpack"
)

build_ig_asset_pack_plan(
  TARGET_NAME ybot-igpack
  PLAN resources/ybot-character.igpack-plan
  INDIR "${asset_root}"
  INFILES
    "characters/y-bot/Jog Forward.fbx"
    "characters/y-bot/Happy Idle.fbx"
  TARGET_OUTPUT_FILES
    "resources/ybot-character.igpack"
    "resources/ybot-basic-animations.igpack"
)

build_ig_asset_pack_plan(
  TARGET_NAME base-shader-igpack
  PLAN "resources/base-shader-sources.igpack-plan"
  INDIR "${asset_root}"
  INFILES
    "engine/debug_3d.vert.wgsl"
    "engine/debug_3d.frag.wgsl"
    "engine/solid_animated.vert.wgsl"
    "engine/solid_animated.frag.wgsl"
  TARGET_OUTPUT_FILES
    "resources/base-shader-sources.igpack"
)

#
# Protocol buffers (for communicating with Sanctify web services)
#
add_proto_lib(
  TARGET_NAME sanctify_api_proto
  OUTDIR "sanctify-game/proto"
  PROTO_INFILES "${PROJECT_SOURCE_DIR}/../proto/sanctify-api.proto"
)

#
# Target definition
#
if (NOT EMSCRIPTEN)
  set(platform_header_list
    "platform_src/netclient/ws_client_native.h")
  set(platform_src_list
    "platform_src/netclient/game_api_native.cc"
    "platform_src/netclient/net_client_native.cc"
    "platform_src/netclient/ws_client_native.cc"
    "platform_src/app_base_native.cc"
    "platform_src/native_main.cc")
else ()
  set(platform_header_list
    "platform_src/netclient/ws_client_web.h")
  set(platform_src_list
    "platform_src/netclient/game_api_web.cc"
    "platform_src/netclient/net_client_web.cc"
    "platform_src/netclient/ws_client_web.cc"
    "platform_src/app_base_web.cc"
    "platform_src/emscripten_bindings.cc")
endif ()

add_executable(sanctify-game-client ${header_list} ${src_list} ${platform_header_list} ${platform_src_list})
target_link_libraries(sanctify-game-client PUBLIC
    igasync igplatform igcore iggpu sanctify-game-common sanctify_api_proto)
target_include_directories(sanctify-game-client PRIVATE platform_src src)

# IGPack dependencies
add_dependencies(sanctify-game-client
  base-shader-igpack
  terrain-geo-igpack
  ybot-igpack)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-game-client EXPORT_NAME SanctifyGameClient)
else ()
  # LEGAL NOTE - use libdatachannel and not libdatachannel-static. This is because
  #  libdatachannel uses the LGPLv2 license and requires that a user be able to
  #  freely link in their own build of the library in its place.
  # Strictly speaking we could probably get around this a different way, but I'd
  #  rather just avoid the headache entirely and link dynamically.
  target_link_libraries(sanctify-game-client
    PUBLIC dawncpp glfw dawn_native dawn_wire
    dawn_utils dawn_proc dawn_common LibDataChannel::LibDataChannel
    http-request)
endif ()

if (WIN32)
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(sanctify-game-client PRIVATE "/Zi")
    target_link_options(sanctify-game-client PRIVATE "/SAFESEH:NO")
  endif()
endif ()

if (MSVC)
  add_custom_command(TARGET sanctify-game-client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:datachannel>/datachannel${CMAKE_DEBUG_POSTFIX}.dll"
    $<TARGET_FILE_DIR:sanctify-game-client>
  )
endif ()
