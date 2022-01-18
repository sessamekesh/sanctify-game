#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_FACTORY_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_FACTORY_H

#include <game_scene/game_scene.h>
#include <igasync/promise.h>

namespace sanctify {

using GamePromise = indigo::core::Promise<std::shared_ptr<GameScene>>;

enum class GameSceneConstructionError {};

std::string to_string(const GameSceneConstructionError& err);

class GameSceneFactory {
 public:
  GameSceneFactory(std::shared_ptr<AppBase> base);

  [[nodiscard("Constructed scene should not be discarded")]] std::shared_ptr<
      GamePromise>
  build() const;

 private:
  std::shared_ptr<AppBase> base_;
};

}  // namespace sanctify

#endif
