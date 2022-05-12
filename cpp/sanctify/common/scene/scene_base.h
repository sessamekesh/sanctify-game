#ifndef SANCTIFY_COMMON_SCENE_SCENE_BASE_H
#define SANCTIFY_COMMON_SCENE_SCENE_BASE_H

#include <webgpu/webgpu_cpp.h>

#include <cstdint>
#include <memory>

namespace sanctify {

class ISceneBase {
 public:
  virtual void update(float dt) {}
  virtual void on_viewport_resize(uint32_t width, uint32_t height) {}
  virtual void on_swap_chain_format_change(wgpu::TextureFormat format) {}
  virtual void render() {}
  virtual bool should_quit() { return false; }

  virtual void attach_io() {}
  virtual void detach_io() {}
};

class ISceneConsumer {
 public:
  virtual void set_scene(std::shared_ptr<ISceneBase> next_scene) {}
};

}  // namespace sanctify

#endif
