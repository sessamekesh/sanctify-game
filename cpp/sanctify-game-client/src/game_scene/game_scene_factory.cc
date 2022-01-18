#include <game_scene/game_scene_factory.h>

using namespace indigo;
using namespace core;
using namespace sanctify;

std::string sanctify::to_string(const GameSceneConstructionError& err) {
  return "";
}

GameSceneFactory::GameSceneFactory(std::shared_ptr<AppBase> base)
    : base_(base) {}

std::shared_ptr<GamePromise> GameSceneFactory::build() const {
  auto scene = std::shared_ptr<GameScene>(new GameScene(base_));

  return GamePromise::immediate(std::move(scene));
}