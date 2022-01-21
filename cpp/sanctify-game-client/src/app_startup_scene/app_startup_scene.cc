#include <app_startup_scene/app_startup_scene.h>
#include <app_startup_scene/startup_shader_src.h>
#include <game_scene/game_scene_factory.h>
#include <igcore/log.h>
#include <iggpu/util.h>

using namespace indigo;
using namespace sanctify;

namespace {
const char* kLogLabel = "AppStartupScene";
}

AppStartupScene::AppStartupScene(
    std::shared_ptr<AppBase> base,
    std::shared_ptr<ISceneConsumer> scene_consumer,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list)
    : base_(base),
      scene_consumer_(scene_consumer),
      main_thread_task_list_(main_thread_task_list),
      render_pipeline_(nullptr),
      bind_group_(nullptr),
      ubo_(base->Device) {
  setup_render_state(*base);

  GameSceneFactory factory(base);
  factory.set_task_lists(main_thread_task_list, async_task_list)
      .build()
      ->on_success(
          [scene_consumer](const GamePromiseRsl& rsl) {
            if (rsl.is_right()) {
              auto log = Logger::err(kLogLabel);

              log << "Error(s) creating game scene:\n";
              for (int i = 0; i < rsl.get_right().size(); i++) {
                log << "--" << to_string(rsl.get_right()[i]);
              }

              // TODO (sessamekesh): Show this message to the user too, if
              // possible

              return;
            }

            scene_consumer->set_scene(rsl.get_left());
          },
          main_thread_task_list);
}

void AppStartupScene::setup_render_state(const AppBase& base) {
  wgpu::ShaderModule vs_module = iggpu::create_shader_module(
      base.Device, sanctify::kStartupShaderVsSrc, "StartupShaderVs");

  wgpu::ShaderModule fs_module = iggpu::create_shader_module(
      base.Device, sanctify::kStartupShaderFsSrc, "StartupShaderFs");

  wgpu::ColorTargetState color_target;
  color_target.format = base.preferred_swap_chain_texture_format();
  color_target.writeMask = wgpu::ColorWriteMask::All;

  wgpu::FragmentState fragment_state{};
  fragment_state.module = fs_module;
  fragment_state.entryPoint = "main";
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target;

  wgpu::RenderPipelineDescriptor pipeline_desc{};
  pipeline_desc.vertex.module = vs_module;
  pipeline_desc.vertex.entryPoint = "main";
  pipeline_desc.fragment = &fragment_state;
  pipeline_desc.primitive.frontFace = wgpu::FrontFace::CCW;
  pipeline_desc.primitive.cullMode = wgpu::CullMode::None;
  pipeline_desc.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;
  pipeline_desc.primitive.stripIndexFormat = wgpu::IndexFormat::Uint32;

  render_pipeline_ = base.Device.CreateRenderPipeline(&pipeline_desc);

  wgpu::BindGroupLayout layout = render_pipeline_.GetBindGroupLayout(0);

  core::Vector<wgpu::BindGroupEntry> entries;
  entries.push_back(
      iggpu::buffer_bind_group_entry(0, ubo_.buffer(), ubo_.size()));
  auto desc = iggpu::bind_group_desc(entries, layout);

  bind_group_ = base.Device.CreateBindGroup(&desc);
}

void AppStartupScene::update(float dt) { ubo_.get_mutable().Time += dt; }

void AppStartupScene::render() {
  const wgpu::Device& device = base_->Device;
  ubo_.get_mutable().Dimensions = glm::vec2(base_->Width, base_->Height);
  ubo_.sync(device);

  // TODO (sessamekesh): Render the loading shader result here fullscreen
  auto command_encoder = device.CreateCommandEncoder();

  wgpu::RenderPassColorAttachment attachment{};
  attachment.clearColor = {0.f, 0.f, 0.f, 0.f};
  attachment.loadOp = wgpu::LoadOp::Clear;
  attachment.storeOp = wgpu::StoreOp::Store;
  attachment.resolveTarget = nullptr;
  attachment.view = base_->SwapChain.GetCurrentTextureView();

  wgpu::RenderPassDescriptor desc{};
  desc.colorAttachmentCount = 1;
  desc.colorAttachments = &attachment;
  desc.depthStencilAttachment = nullptr;

  wgpu::RenderPassEncoder pass = command_encoder.BeginRenderPass(&desc);

  pass.SetPipeline(render_pipeline_);
  pass.SetViewport(0.f, 0.f, base_->Width, base_->Height, 0.f, 1.f);
  pass.SetBindGroup(0, bind_group_);
  pass.Draw(4);
  pass.EndPass();

  wgpu::CommandBuffer buffer = command_encoder.Finish();

  device.GetQueue().Submit(1, &buffer);

#ifndef __EMSCRIPTEN__
  base_->SwapChain.Present();
#endif
}

bool AppStartupScene::should_quit() { return false; }