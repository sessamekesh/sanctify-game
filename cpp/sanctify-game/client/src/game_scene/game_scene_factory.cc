#include <game_scene/factory_util/build_debug_geo_shit.h>
#include <game_scene/factory_util/build_net_client.h>
#include <game_scene/factory_util/build_terrain_shit.h>
#include <game_scene/game_scene_factory.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise_combiner.h>
#include <io/arena_camera_controller/arena_camera_mouse_input_listener.h>
#include <io/viewport_click/viewport_click_mouse_impl.h>
#include <util/scene_setup_util.h>

using namespace indigo;
using namespace core;
using namespace sanctify;

namespace {
const char* kLogLabel = "GameSceneFactory";
}

std::string sanctify::to_string(const GameSceneConstructionError& err) {
#ifndef IG_ENABLE_LOGGING
  return "";
#endif

  switch (err) {
    case GameSceneConstructionError::MissingMainThreadTaskList:
      return "MissingMainThreadTaskList";
    case GameSceneConstructionError::MissingAsyncTaskList:
      return "MissingAsyncTaskList";
    case GameSceneConstructionError::TerrainShitBuildFailed:
      return "TerrainShitBuildFailed";
    case GameSceneConstructionError::DebugGeoShitBuildFailed:
      return "DebugGeoShitBuildFailed";
    case GameSceneConstructionError::NetClientBuildFailed:
      return "NetClientBuildFailed";
    case GameSceneConstructionError::SolidAnimatedPipelineBuilderMissing:
      return "SolidAnimatedPipelineBuilderMissing";
    case GameSceneConstructionError::YBotGeoLoadFail:
      return "YBotGeoLoadFail";
    case GameSceneConstructionError::YBotIdleAnimationLoadFail:
      return "YBotIdleAnimationLoadFail";
    case GameSceneConstructionError::YBotWalkAnimationLoadFail:
      return "YBotWalkAnimationLoadFail";
    case GameSceneConstructionError::YBotSkeletonLoadFail:
      return "YBotSkeletonLoadFail";
  }

  return "UnknownError-" + std::to_string(static_cast<int>(err));
}

GameSceneFactory::GameSceneFactory(std::shared_ptr<AppBase> base)
    : base_(base), main_thread_task_list_(nullptr), async_task_list_(nullptr) {}

GameSceneFactory& GameSceneFactory::set_task_lists(
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list) {
  main_thread_task_list_ = main_thread_task_list;
  async_task_list_ = async_task_list;
  return *this;
}

std::shared_ptr<GamePromise> GameSceneFactory::build() const {
  PodVector<GameSceneConstructionError> missing_input_errors =
      get_missing_inputs_errors();
  if (missing_input_errors.size() > 0) {
    return GamePromise::immediate(right(std::move(missing_input_errors)));
  }

  wgpu::Device device = base_->Device;
  std::shared_ptr<AppBase> base = base_;

  GameScene::GameGeometryKeySet game_geometry_key_set{};
  auto skeleton_registry =
      std::make_shared<ReadonlyResourceRegistry<ozz::animation::Skeleton>>();
  auto animation_registry =
      std::make_shared<ReadonlyResourceRegistry<ozz::animation::Animation>>();
  auto solid_animated_geo_registry = std::make_shared<
      ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>>();

  //
  // Raw loading promise graph nodes
  //
  asset::IgpackLoader terrain_geo_loader("resources/terrain-geo.igpack",
                                         async_task_list_);
  asset::IgpackLoader ybot_character_loader("resources/ybot-character.igpack",
                                            async_task_list_);
  asset::IgpackLoader ybot_animations_loader(
      "resources/ybot-basic-animations.igpack", async_task_list_);
  asset::IgpackLoader base_shader_sources_loader(
      "resources/base-shader-sources.igpack", async_task_list_);

  // LEGACY...
  auto vs_src_promise =
      terrain_geo_loader.extract_wgsl_shader("solidVertWgsl", async_task_list_);
  auto fs_src_promise =
      terrain_geo_loader.extract_wgsl_shader("solidFragWgsl", async_task_list_);
  auto base_geo_promise =
      terrain_geo_loader.extract_draco_geo("arenaBaseGeo", async_task_list_);
  auto decoration_geo_promise = terrain_geo_loader.extract_draco_geo(
      "arenaDecorationsGeo", async_task_list_);
  auto mid_tower_geo_promiose =
      terrain_geo_loader.extract_draco_geo("midTowerGeo", async_task_list_);

  auto debug_geo_vs_src_promise =
      base_shader_sources_loader.extract_wgsl_shader("debug3dVertWgsl",
                                                     async_task_list_);
  auto debug_geo_fs_src_promise =
      base_shader_sources_loader.extract_wgsl_shader("debug3dFragWgsl",
                                                     async_task_list_);

  auto terrain_shit_promise = ::load_terrain_shit(
      device, base_->preferred_swap_chain_texture_format(), vs_src_promise,
      fs_src_promise, base_geo_promise, decoration_geo_promise,
      mid_tower_geo_promiose, main_thread_task_list_, async_task_list_);
  auto debug_geo_shit_promise = ::load_debug_geo_shit(
      device, base_->preferred_swap_chain_texture_format(),
      debug_geo_vs_src_promise, debug_geo_fs_src_promise,
      main_thread_task_list_, async_task_list_);

  //
  // Solid animated pipeline...
  //
  auto maybe_solid_animated_pipeline_promise =
      util::load_solid_animated_pipeline(
          device,
          base_shader_sources_loader.extract_wgsl_shader(
              "solidAnimatedVertWgsl", async_task_list_),
          base_shader_sources_loader.extract_wgsl_shader(
              "solidAnimatedFragWgsl", async_task_list_),
          main_thread_task_list_, async_task_list_);

  //
  // Character models + animations...
  //
  auto ybot_skeleton_create_pair = util::load_ozz_skeleton(
      ybot_animations_loader, "ybotSkeleton", skeleton_registry,
      main_thread_task_list_, async_task_list_);
  auto ybot_idle_anim_create_pair = util::load_ozz_animation(
      ybot_animations_loader, "ybotIdle", animation_registry,
      main_thread_task_list_, async_task_list_);
  auto ybot_walk_anim_create_pair = util::load_ozz_animation(
      ybot_animations_loader, "ybotJog", animation_registry,
      main_thread_task_list_, async_task_list_);
  auto ybot_base_geo_create_pair = util::load_geo(
      device,
      ybot_character_loader.extract_draco_geo("ybotBase", async_task_list_),
      "Ybot Base Geo", solid_animated_geo_registry, skeleton_registry,
      ybot_skeleton_create_pair.key, ybot_skeleton_create_pair.result_promise,
      main_thread_task_list_, async_task_list_);
  auto ybot_joints_geo_create_pair = util::load_geo(
      device,
      ybot_character_loader.extract_draco_geo("ybotJoints", async_task_list_),
      "Ybot Joints Geo", solid_animated_geo_registry, skeleton_registry,
      ybot_skeleton_create_pair.key, ybot_skeleton_create_pair.result_promise,
      main_thread_task_list_, async_task_list_);

  game_geometry_key_set.ybotBaseGeoKey = ybot_base_geo_create_pair.key;
  game_geometry_key_set.ybotIdleAnimationKey = ybot_idle_anim_create_pair.key;
  game_geometry_key_set.ybotJointsGeoKey = ybot_joints_geo_create_pair.key;
  game_geometry_key_set.ybotSkeletonKey = ybot_skeleton_create_pair.key;
  game_geometry_key_set.ybotWalkAnimationKey = ybot_walk_anim_create_pair.key;

  // TODO (sessamekesh): Remove this hardcoded URL base
#ifdef __EMSCRIPTEN__
  std::string game_api_url = "http://localhost:3000";
#else
  std::string game_api_url = "http://localhost:8080";
#endif
  auto net_client_promise = ::build_net_client(game_api_url, async_task_list_);

  //
  // Create combiner and wrangle together parameter inputs (use util files for
  //  most of these to prevent cluttering this file
  //
  auto combiner = PromiseCombiner::Create();
  auto terrain_shit_key = combiner->add(terrain_shit_promise, async_task_list_);
  auto debug_geo_shit_key =
      combiner->add(debug_geo_shit_promise, async_task_list_);
  auto solid_animated_pipeline_key =
      combiner->add(maybe_solid_animated_pipeline_promise, async_task_list_);
  auto net_client_key = combiner->add(net_client_promise, async_task_list_);

  auto ybot_base_geo_ok_key =
      combiner->add(ybot_base_geo_create_pair.result_promise, async_task_list_);
  auto ybot_joint_geo_ok_key = combiner->add(
      ybot_joints_geo_create_pair.result_promise, async_task_list_);
  auto ybot_skele_ok_key =
      combiner->add(ybot_skeleton_create_pair.result_promise, async_task_list_);
  auto ybot_walk_ok_key = combiner->add(
      ybot_walk_anim_create_pair.result_promise, async_task_list_);
  auto ybot_idle_ok_key = combiner->add(
      ybot_idle_anim_create_pair.result_promise, async_task_list_);

  return combiner->combine()->then<GamePromiseRsl>(
      [base, terrain_shit_key, net_client_key, debug_geo_shit_key,
       solid_animated_pipeline_key, ybot_base_geo_ok_key, ybot_joint_geo_ok_key,
       ybot_skele_ok_key, ybot_walk_ok_key, ybot_idle_ok_key,
       game_geometry_key_set, animation_registry, solid_animated_geo_registry,
       skeleton_registry](
          const PromiseCombiner::PromiseCombinerResult& rsl) -> GamePromiseRsl {
        //
        // Extract intermediate results
        //
        Maybe<GameScene::TerrainShit> terrain_shit_rsl =
            rsl.move(terrain_shit_key);
        Maybe<GameScene::DebugGeoShit> debug_geso_shit_rsl =
            rsl.move(debug_geo_shit_key);
        Maybe<std::shared_ptr<NetClient>> net_client_rsl =
            rsl.move(net_client_key);
        Maybe<solid_animated::SolidAnimatedPipelineBuilder> solid_animated_rsl =
            rsl.move(solid_animated_pipeline_key);

        core::PodVector<GameSceneConstructionError> result_extract_errors;
        if (terrain_shit_rsl.is_empty()) {
          result_extract_errors.push_back(
              GameSceneConstructionError::TerrainShitBuildFailed);
        }
        if (debug_geso_shit_rsl.is_empty()) {
          result_extract_errors.push_back(
              GameSceneConstructionError::DebugGeoShitBuildFailed);
        }
        if (net_client_rsl.is_empty()) {
          result_extract_errors.push_back(
              GameSceneConstructionError::NetClientBuildFailed);
        }
        if (solid_animated_rsl.is_empty()) {
          result_extract_errors.push_back(
              GameSceneConstructionError::SolidAnimatedPipelineBuilderMissing);
        }
        if (!rsl.get(ybot_base_geo_ok_key) || !rsl.get(ybot_joint_geo_ok_key)) {
          result_extract_errors.push_back(
              GameSceneConstructionError::YBotGeoLoadFail);
        }
        if (!rsl.get(ybot_skele_ok_key)) {
          result_extract_errors.push_back(
              GameSceneConstructionError::YBotSkeletonLoadFail);
        }
        if (!rsl.get(ybot_idle_ok_key)) {
          result_extract_errors.push_back(
              GameSceneConstructionError::YBotIdleAnimationLoadFail);
        }
        if (!rsl.get(ybot_walk_ok_key)) {
          result_extract_errors.push_back(
              GameSceneConstructionError::YBotWalkAnimationLoadFail);
        }

        if (result_extract_errors.size() > 0) {
          return right(std::move(result_extract_errors));
        }

        //
        // Final resource creation
        //
        ArenaCamera arena_camera(glm::vec3(0.f, 0.f, 0.f), glm::radians(30.f),
                                 glm::radians(45.f), 85.f);

        // TODO (sessamekesh): inject this configurably, not always with this
        std::shared_ptr<IArenaCameraInput> arena_camera_input =
            ArenaCameraMouseInputListener::Create(base->Window);
        std::shared_ptr<IViewportClickControllerInput>
            viewport_click_controller =
                ViewportClickMouseImpl::Create(base->Window);
        std::shared_ptr<ViewportClickInput> input =
            std::make_shared<ViewportClickInput>(viewport_click_controller);

        return left(GameScene::Create(
            base, arena_camera, arena_camera_input, input,
            terrain_shit_rsl.move(), solid_animated_rsl.move(),
            game_geometry_key_set, debug_geso_shit_rsl.move(),
            solid_animated_geo_registry, skeleton_registry, animation_registry,
            net_client_rsl.get(), 0.8f, glm::radians(40.f)));
      },
      async_task_list_);
}

PodVector<GameSceneConstructionError>
GameSceneFactory::get_missing_inputs_errors() const {
  PodVector<GameSceneConstructionError> errors;

  if (!main_thread_task_list_) {
    errors.push_back(GameSceneConstructionError::MissingMainThreadTaskList);
  }

  if (!async_task_list_) {
    errors.push_back(GameSceneConstructionError::MissingAsyncTaskList);
  }

  return errors;
}