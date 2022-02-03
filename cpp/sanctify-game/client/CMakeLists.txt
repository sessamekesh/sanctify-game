#
# Sources (most often modified, keep at top)
#
set(header_list
  "platform_src/app_base.h"
  "src/app_startup_scene/app_startup_scene.h"
  "src/app_startup_scene/startup_shader_src.h"
  "src/game_scene/factory_util/build_terrain_shit.h"
  "src/game_scene/factory_util/build_character_geo.h"
  "src/game_scene/game_scene.h"
  "src/game_scene/game_scene_factory.h"
  "src/io/arena_camera_controller/arena_camera_input.h"
  "src/io/arena_camera_controller/arena_camera_mouse_input_listener.h"
  "src/io/glfw_event_emitter.h"
  "src/render/camera/arena_camera.h"
  "src/render/solid_animated/solid_animated_geo.h"
  "src/render/solid_animated/solid_animated_pipeline.h"
  "src/render/terrain/terrain_geo.h"
  "src/render/terrain/terrain_pipeline.h"
  "src/util/camera_util.h"
  "src/sanctify_client_app.h")

set(src_list
  "src/game_scene/factory_util/build_terrain_shit.cc"
  "src/game_scene/factory_util/build_character_geo.cc"
  "src/app_startup_scene/app_startup_scene.cc"
  "src/game_scene/game_scene.cc"
  "src/game_scene/game_scene_factory.cc"
  "src/io/arena_camera_controller/arena_camera_mouse_input_listener.cc"
  "src/io/glfw_event_emitter.cc"
  "src/render/camera/arena_camera.cc"
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
    "sanctify_arena/sanctify-arena.fbx"
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
  TARGET_OUTPUT_FILES
    "resources/ybot-character.igpack"
)

build_ig_asset_pack_plan(
  TARGET_NAME base-shader-igpack
  PLAN "resources/base-shader-sources.igpack-plan"
  INDIR "${asset_root}"
  INFILES
    "engine/solid_animated.vert.wgsl"
    "engine/solid_animated.frag.wgsl"
  TARGET_OUTPUT_FILES
    "resources/base-shader-sources.igpack"
)

#
# Target definition
#
if (NOT EMSCRIPTEN)
  set(platform_src_list
    "platform_src/app_base_native.cc"
    "platform_src/native_main.cc")
else ()
  set(platform_src_list
    "platform_src/app_base_web.cc"
    "platform_src/emscripten_bindings.cc")
endif ()

add_executable(sanctify-game-client ${header_list} ${src_list} ${platform_src_list})
target_link_libraries(sanctify-game-client PUBLIC igasync igplatform igcore iggpu sanctify-game-common)
target_include_directories(sanctify-game-client PRIVATE platform_src src)

# IGPack dependencies
add_dependencies(sanctify-game-client
  base-shader-igpack
  terrain-geo-igpack
  ybot-igpack)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-game-client EXPORT_NAME SanctifyGameClient)
else ()
  target_link_libraries(sanctify-game-client
    PUBLIC dawncpp glfw dawn_native dawn_wire
    dawn_utils dawn_proc dawn_common)  
endif ()

if (WIN32)
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(sanctify-game-client PRIVATE "/Zi")
    target_link_options(sanctify-game-client PRIVATE "/SAFESEH:NO")
  endif()
endif ()