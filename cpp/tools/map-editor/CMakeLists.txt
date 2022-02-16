set(INCLUDE_LIST
  "src/util/assimp_loader.h"
  "src/util/recast_params.h"
  "src/views/navmesh_params_view.h"
  "src/views/viewport_view.h"
  "src/app_window.h"
  "src/map_editor_app.h")

set(SRC_LIST
  "src/util/assimp_loader.cc"
  "src/util/recast_params.cc"
  "src/views/navmesh_params_view.cc"
  "src/views/viewport_view.cc"
  "src/app_window.cc"
  "src/map_editor_app.cc"

  "main.cc")

add_executable(map-editor ${INCLUDE_LIST} ${SRC_LIST})

target_link_libraries(map-editor PUBLIC
    igasset igasset_proto igpack_plan_proto draco_lib CLI11
    assimp dawncpp glfw dawn_native dawn_wire dawn_utils
    dawn_proc imgui iggpu nativefiledialog)

target_include_directories(map-editor PRIVATE src "${PROJECT_SOURCE_DIR}/extern/draco/src" .)

#
# Asset Packs
#
set(asset_root "${PROJECT_SOURCE_DIR}/../assets")

build_ig_asset_pack_plan(
  TARGET_NAME map-gen-base-igpack
  PLAN "resources/base-app.igpack-plan"
  INDIR "${asset_root}"
  INFILES
    "tooling/grid.vs.wgsl"
    "tooling/grid.fs.wgsl"
  TARGET_OUTPUT_FILES
    "resources/base-app.igpack"
)

add_dependencies(map-editor map-gen-base-igpack)