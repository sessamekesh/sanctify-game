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
  "src/ecs/components/common_render_components.h"
  "src/ecs/components/debug_geo_render_components.h"
  "src/ecs/components/parent_entity_component.h"
  "src/ecs/components/solid_animated_components.h"
  "src/ecs/components/terrain_render_components.h"
  "src/ecs/systems/attach_player_renderables.h"
  "src/ecs/systems/destroy_children_system.h"
  "src/ecs/systems/locomotion_blend_system.h"
  "src/ecs/systems/solid_animation_systems.h"
  "src/ecs/utils/debug_geo_render_utils.h"
  "src/ecs/utils/terrain_render_utils.h"
  "src/game_scene/factory_util/build_debug_geo_shit.h"
  "src/game_scene/factory_util/build_net_client.h"
  "src/game_scene/factory_util/build_terrain_shit.h"
  "src/game_scene/systems/player_move_indicator_render_system.h"
  "src/game_scene/game_scene.h"
  "src/game_scene/game_scene_factory.h"
  "src/io/arena_camera_controller/arena_camera_input.h"
  "src/io/arena_camera_controller/arena_camera_mouse_input_listener.h"
  "src/io/viewport_click/viewport_click_controller_input.h"
  "src/io/viewport_click/viewport_click_mouse_impl.h"
  "src/io/glfw_event_emitter.h"
  "src/net/reconcile_net_state_system.h"
  "src/net/snapshot_cache.h"
  "src/pve_game_scene/ecs/camera.h"
  "src/pve_game_scene/ecs/client_config.h"
  "src/pve_game_scene/ecs/movement_indicator_render_system.h"
  "src/pve_game_scene/ecs/netsync_components.h"
  "src/pve_game_scene/ecs/sim_time_sync_system.h"
  "src/pve_game_scene/ecs/utils.h"
  "src/pve_game_scene/io/glfw_io_system.h"
  "src/pve_game_scene/io/io_system.h"
  "src/pve_game_scene/render/common_resources.h"
  "src/pve_game_scene/render/debug_geo_resources.h"
  "src/pve_game_scene/render/loading_screen.h"
  "src/pve_game_scene/render/render_game.h"
  "src/pve_game_scene/render/terrain_resources.h"
  "src/pve_game_scene/render/update_common_buffers_system.h"
  "src/pve_game_scene/build.h"
  "src/pve_game_scene/pve_game_scene.h"
  "src/pve_game_scene/pve_scene_load.h"
  "src/render/camera/arena_camera.h"
  "src/render/common/camera_ubo.h"
  "src/render/debug_geo/debug_geo.h"
  "src/render/debug_geo/debug_geo_pipeline.h"
  "src/render/solid_animated/solid_animated_geo.h"
  "src/render/solid_animated/solid_animated_pipeline.h"
  "src/render/terrain/terrain_geo.h"
  "src/render/terrain/terrain_instance_buffer_store.h"
  "src/render/terrain/terrain_pipeline.h"
  "src/util/camera_util.h"
  "src/util/logical_to_render_utils.h"
  "src/util/registry_types.h"
  "src/util/resource_registry.h"
  "src/util/scene_setup_util.h"
  "src/sanctify_client_app.h")

set(src_list
  "platform_src/netclient/game_api.cc"
  "platform_src/netclient/net_client.cc"
  "platform_src/netclient/ws_client_base.cc"
  "src/game_scene/factory_util/build_debug_geo_shit.cc"
  "src/game_scene/factory_util/build_net_client.cc"
  "src/game_scene/factory_util/build_terrain_shit.cc"
  "src/app_startup_scene/app_startup_scene.cc"
  "src/app_startup_scene/startup_shader_src.cc"
  "src/ecs/systems/attach_player_renderables.cc"
  "src/ecs/systems/destroy_children_system.cc"
  "src/ecs/systems/locomotion_blend_system.cc"
  "src/ecs/systems/solid_animation_systems.cc"
  "src/ecs/utils/debug_geo_render_utils.cc"
  "src/ecs/utils/terrain_render_utils.cc"
  "src/game_scene/systems/player_move_indicator_render_system.cc"
  "src/game_scene/game_scene.cc"
  "src/game_scene/game_scene_factory.cc"
  "src/io/arena_camera_controller/arena_camera_mouse_input_listener.cc"
  "src/io/viewport_click/viewport_click_controller_input.cc"
  "src/io/viewport_click/viewport_click_mouse_impl.cc"
  "src/io/glfw_event_emitter.cc"
  "src/net/reconcile_net_state_system.cc"
  "src/net/snapshot_cache.cc"
  "src/pve_game_scene/ecs/camera.cc"
  "src/pve_game_scene/ecs/movement_indicator_render_system.cc"
  "src/pve_game_scene/ecs/sim_time_sync_system.cc"
  "src/pve_game_scene/ecs/utils.cc"
  "src/pve_game_scene/io/glfw_io_system.cc"
  "src/pve_game_scene/io/io_system.cc"
  "src/pve_game_scene/render/common_resources.cc"
  "src/pve_game_scene/render/debug_geo_resources.cc"
  "src/pve_game_scene/render/loading_screen.cc"
  "src/pve_game_scene/render/render_game.cc"
  "src/pve_game_scene/render/terrain_resources.cc"
  "src/pve_game_scene/render/update_common_buffers_system.cc"
  "src/pve_game_scene/build.cc"
  "src/pve_game_scene/pve_game_scene.cc"
  "src/pve_game_scene/pve_scene_load.cc"
  "src/render/camera/arena_camera.cc"
  "src/render/debug_geo/debug_geo.cc"
  "src/render/debug_geo/debug_geo_pipeline.cc"
  "src/render/solid_animated/solid_animated_geo.cc"
  "src/render/solid_animated/solid_animated_pipeline.cc"
  "src/render/terrain/terrain_geo.cc"
  "src/render/terrain/terrain_instance_buffer_store.cc"
  "src/render/terrain/terrain_pipeline.cc"
  "src/util/camera_util.cc"
  "src/util/logical_to_render_utils.cc"
  "src/util/resource_registry.cc"
  "src/util/scene_setup_util.cc"
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
    "sanctify_arena/sanctify-pve.fbx"
    "engine/solid_3d.vert.wgsl"
    "engine/solid_3d.frag.wgsl"
  TARGET_OUTPUT_FILES
    "resources/terrain-geo.igpack"
    "resources/pve-terrain-geo.igpack"
    "resources/terrain-shaders.igpack"
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

build_ig_asset_pack_plan(
  TARGET_NAME pve-terrain-igpack-client
  PLAN "${PROJECT_SOURCE_DIR}/sanctify-game/common/resources/pve-terrain-navmesh.igpack-plan"
  INDIR "${asset_root}"
  INFILES
    "sanctify_arena/sanctify-pve.fbx"
  TARGET_OUTPUT_FILES
    "resources/terrain-pve.igpack"
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
  ybot-igpack
  pve-terrain-igpack-client)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-game-client EXPORT_NAME SanctifyGameClient)
else ()
  find_package(Boost 1.67 REQUIRED COMPONENTS system thread random)
  target_include_directories(sanctify-game-client PRIVATE
    . "${websocketpp_SOURCE_DIR}")

  # LEGAL NOTE - use libdatachannel and not libdatachannel-static. This is because
  #  libdatachannel uses the LGPLv2 license and requires that a user be able to
  #  freely link in their own build of the library in its place.
  # Strictly speaking we could probably get around this a different way, but I'd
  #  rather just avoid the headache entirely and link dynamically.
  target_link_libraries(sanctify-game-client
    PUBLIC dawncpp glfw dawn_native dawn_wire
    dawn_utils dawn_proc dawn_common LibDataChannel::LibDataChannel
    http-request Boost::boost ${Boost_LIBRARIES})
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
