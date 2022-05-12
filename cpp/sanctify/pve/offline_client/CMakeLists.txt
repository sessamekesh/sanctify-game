set(hdr_list
  "game_scene/systems/process_user_input.h"
  "game_scene/systems/stage_client_message.h"
  "game_scene/pve_offline_game_scene.h"
  "game_scene/pve_offline_render.h"
  "game_scene/render_client_scheduler.h"
  "game_scene/update_client_scheduler.h"
  "offline_client_app.h")

set(src_list
  "game_scene/systems/process_user_input.cc"
  "game_scene/systems/stage_client_message.cc"
  "game_scene/pve_offline_game_scene.cc"
  "game_scene/pve_offline_render.cc"
  "game_scene/render_client_scheduler.cc"
  "game_scene/update_client_scheduler.cc"
  "offline_client_app.cc")

if (EMSCRIPTEN)
  set(platform_src_list
    "embind_main.cc")
else ()
  set(platform_src_list
    "native_main.cc")
endif () 

add_proto_lib(
  TARGET_NAME sanctify-pve-offline-client-config-proto
  OUTDIR "pve/offline_client/pb"
  PROTO_INFILES "pve_offline_client_config.proto"
)

#
# Asset Packs
#
set(asset_root "${PROJECT_SOURCE_DIR}/../assets")

build_ig_asset_pack_plan(
  TARGET_NAME  sanctify-pve-offline-common-shaders-igpack
  PLAN         resources/common-shaders.igpack-plan
  INDIR        "${SANCTIFY_INCLUDE_ROOT}"
  INFILES
    "common/render/solid_static/solid_static.vs.wgsl"
    "common/render/solid_static/solid_static.fs.wgsl"
    "common/render/tonemap/tonemap.vs.wgsl"
    "common/render/tonemap/tonemap.fs.wgsl"
  TARGET_OUTPUT_FILES
    "resources/common-shaders.igpack"
)

build_ig_asset_pack_plan(
  TARGET_NAME  sanctify-pve-offline-arena-base-igpack
  PLAN         resources/arena-base.igpack-plan
  INDIR        "${asset_root}"
  INFILES
    "sanctify_arena/sanctify-pve.fbx"
  TARGET_OUTPUT_FILES
    "resources/arena-base.igpack"
)

add_executable(sanctify-pve-offline-client ${platform_src_list} ${hdr_list} ${src_list})
target_include_directories(sanctify-simple-client-app PUBLIC "${SANCTIFY_INCLUDE_ROOT}")
target_link_libraries(sanctify-pve-offline-client PUBLIC
            sanctify-common-logic
            sanctify-common-render
            sanctify-common-scene
            sanctify-common-user-input
            sanctify-pve-net-logic-common
            sanctify-pve-offline-client-config-proto
            sanctify-pve-render-common
            sanctify-simple-client-app)
add_dependencies(sanctify-pve-offline-client
            sanctify-pve-offline-common-shaders-igpack
            sanctify-pve-offline-arena-base-igpack)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-pve-offline-client
                             EXPORT_NAME SanctifyPveOfflineClient)
endif ()
