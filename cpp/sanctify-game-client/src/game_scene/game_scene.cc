#include <game_scene/game_scene.h>

using namespace sanctify;

GameScene::GameScene(std::shared_ptr<AppBase> base) : base_(base) {}

void GameScene::update(float dt) {}

void GameScene::render() {}

bool GameScene::should_quit() { return false; }