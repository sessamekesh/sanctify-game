#include <game_scene/factory_util/build_character_geo.h>
#include <game_scene/factory_util/build_terrain_shit.h>
#include <game_scene/game_scene_factory.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise_combiner.h>
#include <io/arena_camera_controller/arena_camera_mouse_input_listener.h>

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
    case GameSceneConstructionError::PlayerShitBuildFailed:
      return "PlayerShitBuildFailed";
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

  //
  // Raw loading promise graph nodes
  //
  asset::IgpackLoader terrain_geo_loader("resources/terrain-geo.igpack",
                                         async_task_list_);
  asset::IgpackLoader ybot_character_loader("resources/ybot-character.igpack",
                                            async_task_list_);
  asset::IgpackLoader base_shader_sources_loader(
      "resources/base-shader-sources.igpack", async_task_list_);

  auto vs_src_promise =
      terrain_geo_loader.extract_wgsl_shader("solidVertWgsl", async_task_list_);
  auto fs_src_promise =
      terrain_geo_loader.extract_wgsl_shader("solidFragWgsl", async_task_list_);
  auto base_geo_promise =
      terrain_geo_loader.extract_draco_geo("arenaBaseGeo", async_task_list_);
  auto decoration_geo_promise = terrain_geo_loader.extract_draco_geo(
      "arenaDecorationsGeo", async_task_list_);

  auto solid_animated_vs_src_promise =
      base_shader_sources_loader.extract_wgsl_shader("solidAnimatedVertWgsl",
                                                     async_task_list_);
  auto solid_animated_fs_src_promise =
      base_shader_sources_loader.extract_wgsl_shader("solidAnimatedFragWgsl",
                                                     async_task_list_);
  auto ybot_surface_geo_promise =
      ybot_character_loader.extract_draco_geo("ybotBase", async_task_list_);
  auto ybot_joints_geo_promise =
      ybot_character_loader.extract_draco_geo("ybotJoints", async_task_list_);

  auto terrain_shit_promise = ::load_terrain_shit(
      device, base_->preferred_swap_chain_texture_format(), vs_src_promise,
      fs_src_promise, base_geo_promise, decoration_geo_promise,
      main_thread_task_list_, async_task_list_);
  auto player_shit_promise = ::load_player_shit(
      device, base_->preferred_swap_chain_texture_format(),
      solid_animated_vs_src_promise, solid_animated_fs_src_promise,
      ybot_surface_geo_promise, ybot_joints_geo_promise, main_thread_task_list_,
      async_task_list_);

  //
  // Create combiner and wrangle together parameter inputs (use util files for
  //  most of these to prevent cluttering this file
  //
  auto combiner = PromiseCombiner::Create();
  auto terrain_shit_key = combiner->add(terrain_shit_promise, async_task_list_);
  auto player_shit_key = combiner->add(player_shit_promise, async_task_list_);

  return combiner->combine()->then<GamePromiseRsl>(
      [base, terrain_shit_key, player_shit_key](
          const PromiseCombiner::PromiseCombinerResult& rsl) -> GamePromiseRsl {
        //
        // Extract intermediate results
        //
        Maybe<GameScene::TerrainShit> terrain_shit_rsl =
            rsl.move(terrain_shit_key);
        Maybe<GameScene::PlayerShit> player_shit_rsl =
            rsl.move(player_shit_key);

        core::PodVector<GameSceneConstructionError> result_extract_errors;
        if (terrain_shit_rsl.is_empty()) {
          result_extract_errors.push_back(
              GameSceneConstructionError::TerrainShitBuildFailed);
        }
        if (player_shit_rsl.is_empty()) {
          result_extract_errors.push_back(
              GameSceneConstructionError::PlayerShitBuildFailed);
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

        return left(std::shared_ptr<GameScene>(new GameScene(
            base, arena_camera, arena_camera_input, terrain_shit_rsl.move(),
            player_shit_rsl.move(), 0.8f, glm::radians(40.f))));
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