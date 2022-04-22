#ifndef SANCTIFY_COMMON_SCENE_PRELOAD_PRELOAD_SCENE_H
#define SANCTIFY_COMMON_SCENE_PRELOAD_PRELOAD_SCENE_H

#include <igasync/promise.h>
#include <igasync/task_list.h>
#include <igcore/either.h>
#include <iggpu/ubo_base.h>

#include "../scene_base.h"
#include "preload_shader.h"

/**
 * Preload scene: compiles with all requirements and does not need any async
 *  resources. Useful for running while run-time loaded resources are in flight.
 */

namespace sanctify {

class PreloadScene : public ISceneBase {
 public:
  PreloadScene(
      wgpu::Device device, wgpu::SwapChain swap_chain, uint32_t vp_width,
      uint32_t vp_height, wgpu::TextureFormat swap_chain_format,
      std::shared_ptr<ISceneConsumer> scene_consumer,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
      std::shared_ptr<indigo::core::Promise<
          indigo::core::Either<std::shared_ptr<ISceneBase>, std::string>>>
          next_scene_promise);

  // ISceneBase
  void update(float dt) override;
  void render() override;
  bool should_quit() override;
  void on_viewport_resize(uint32_t width, uint32_t height) override;
  void on_swap_chain_format_change(wgpu::TextureFormat format) override;

 private:
  void create_render_pipeline();

 private:
  // Injected platform details
  wgpu::Device device_;
  wgpu::SwapChain swap_chain_;
  wgpu::TextureFormat swap_chain_format_;
  uint32_t vp_width_, vp_height_;

  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;
  std::shared_ptr<ISceneConsumer> scene_consumer_;

  // Render resources
  wgpu::RenderPipeline render_pipeline_;
  indigo::iggpu::UboBase<PreloadShader::FsParams> fs_ubo_;
  wgpu::BindGroup bind_group_;
};

}  // namespace sanctify

#endif
