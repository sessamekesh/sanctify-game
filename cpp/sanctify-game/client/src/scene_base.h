#ifndef SANCTIFY_GAME_CLIENT_SRC_ISCENEBASE_H
#define SANCTIFY_GAME_CLIENT_SRC_ISCENEBASE_H

#include <app_base.h>

class ISceneBase {
 public:
  virtual void update(float dt) {}
  virtual void on_viewport_resize(uint32_t width, uint32_t height) {}
  virtual void render() {}
  virtual bool should_quit() { return false; }
};

class ISceneConsumer {
 public:
  virtual void set_scene(std::shared_ptr<ISceneBase> next_scene) = 0;
};

#endif
