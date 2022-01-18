#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_H

#include <igasync/promise.h>
#include <scene_base.h>

namespace sanctify {

class GameScene : public ISceneBase {
 public:
  GameScene(std::shared_ptr<AppBase> base);

  // ISceneBase
  void update(float dt) override;
  void render() override;
  bool should_quit() override;

 private:
  std::shared_ptr<AppBase> base_;
};

}  // namespace sanctify

#endif
