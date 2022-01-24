#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_FACTORY_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_FACTORY_H

#include <game_scene/game_scene.h>
#include <igasync/promise.h>
#include <igasync/task_list.h>
#include <igcore/either.h>

namespace sanctify {

enum class GameSceneConstructionError {
  MissingMainThreadTaskList,
  MissingAsyncTaskList,

  TerrainShitBuildFailed,
  PlayerShitBuildFailed,
};

using GamePromiseRsl =
    indigo::core::Either<std::shared_ptr<GameScene>,
                         indigo::core::PodVector<GameSceneConstructionError>>;

using GamePromise = indigo::core::Promise<GamePromiseRsl>;

std::string to_string(const GameSceneConstructionError& err);

class GameSceneFactory {
 public:
  GameSceneFactory(std::shared_ptr<AppBase> base);

  GameSceneFactory& set_task_lists(
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
      std::shared_ptr<indigo::core::TaskList> async_task_list);

  [[nodiscard("Constructed scene should not be discarded")]] std::shared_ptr<
      GamePromise>
  build() const;

 private:
  indigo::core::PodVector<GameSceneConstructionError>
  get_missing_inputs_errors() const;

  std::shared_ptr<AppBase> base_;
  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;
  std::shared_ptr<indigo::core::TaskList> async_task_list_;
};

}  // namespace sanctify

#endif
