#include "preload_scene.h"

using namespace sanctify;

using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "PreloadScene";
}

PreloadScene::PreloadScene(
    wgpu::Device device, wgpu::SwapChain swap_chain, uint32_t vp_width,
    uint32_t vp_height, wgpu::TextureFormat swap_chain_format,
    std::shared_ptr<ISceneConsumer> scene_consumer,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<Promise<Either<std::shared_ptr<ISceneBase>, std::string>>>
        next_scene_promise)
    : device_(device),
      swap_chain_(swap_chain),
      vp_width_(vp_width),
      vp_height_(vp_height),
      swap_chain_format_(swap_chain_format),
      main_thread_task_list_(main_thread_task_list),
      scene_consumer_(scene_consumer),
      render_pipeline_(nullptr),  // TODO (sessamekesh)
      fs_ubo_(device),
      bind_group_(nullptr)  // TODO (sessamekesh)
{
  next_scene_promise->on_success(
      [scene_consumer](
          const Either<std::shared_ptr<ISceneBase>, std::string>& rsl) {
        if (rsl.is_right()) {
          Logger::err(kLogLabel)
              << "Failed to load next scene: " << rsl.get_right();
        }

        scene_consumer->set_scene(rsl.get_left());
      },
      main_thread_task_list_);

  fs_ubo_.get_mutable().t =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count() %
      123456789;

  create_render_pipeline();
}

void PreloadScene::update(float dt) {
  auto& ubo_data = fs_ubo_.get_mutable();
  ubo_data.t += dt;
}

void PreloadScene::render() {
  {
    auto& ubo_data = fs_ubo_.get_mutable();
    ubo_data.pathRadius = 300.f;
    ubo_data.ballSize = 8.f;
    ubo_data.viewDimensions = glm::vec2(vp_width_, vp_height_);
  }
  fs_ubo_.sync(device_);

  auto command_encoder = device_.CreateCommandEncoder();

  wgpu::RenderPassColorAttachment attachment{};
  attachment.clearValue = {0.f, 0.f, 0.f, 0.f};
  attachment.loadOp = wgpu::LoadOp::Clear;
  attachment.storeOp = wgpu::StoreOp::Store;
  attachment.resolveTarget = nullptr;
  attachment.view = swap_chain_.GetCurrentTextureView();

  wgpu::RenderPassDescriptor desc{};
  desc.colorAttachmentCount = 1;
  desc.colorAttachments = &attachment;
  desc.depthStencilAttachment = nullptr;

  wgpu::RenderPassEncoder pass = command_encoder.BeginRenderPass(&desc);

  pass.SetPipeline(render_pipeline_);
  pass.SetViewport(0.f, 0.f, (float)vp_width_, (float)vp_height_, 0.f, 1.f);
  pass.SetBindGroup(0, bind_group_);
  pass.Draw(4);
  pass.End();

  wgpu::CommandBuffer buffer = command_encoder.Finish();

  device_.GetQueue().Submit(1, &buffer);

  swap_chain_.Present();
}

bool PreloadScene::should_quit() { return false; }

void PreloadScene::on_viewport_resize(uint32_t width, uint32_t height) {
  vp_width_ = width;
  vp_height_ = height;
}

void PreloadScene::on_swap_chain_format_change(wgpu::TextureFormat format) {
  swap_chain_format_ = format;
  create_render_pipeline();
}

void PreloadScene::create_render_pipeline() {
  wgpu::ShaderModule vs_mod = iggpu::create_shader_module(
      device_, sanctify::kPreloadShaderVsSrc, "PreloadShaderVs");
  wgpu::ShaderModule fs_mod = iggpu::create_shader_module(
      device_, sanctify::kPreloadShaderFsSrc, "PreloadShaderFs");

  wgpu::ColorTargetState color_target{};
  color_target.format = swap_chain_format_;
  color_target.writeMask = wgpu::ColorWriteMask::All;

  wgpu::FragmentState frag_state{};
  frag_state.module = fs_mod;
  frag_state.entryPoint = "main";
  frag_state.targetCount = 1;
  frag_state.targets = &color_target;

  wgpu::RenderPipelineDescriptor pipeline_desc{};
  pipeline_desc.vertex.module = vs_mod;
  pipeline_desc.vertex.entryPoint = "main";
  pipeline_desc.fragment = &frag_state;
  pipeline_desc.primitive.frontFace = wgpu::FrontFace::CCW;
  pipeline_desc.primitive.cullMode = wgpu::CullMode::None;
  pipeline_desc.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;
  pipeline_desc.primitive.stripIndexFormat = wgpu::IndexFormat::Uint16;

  render_pipeline_ = device_.CreateRenderPipeline(&pipeline_desc);

  wgpu::BindGroupLayout layout = render_pipeline_.GetBindGroupLayout(0);

  core::Vector<wgpu::BindGroupEntry> entries;
  entries.push_back(
      iggpu::buffer_bind_group_entry(0, fs_ubo_.buffer(), fs_ubo_.size()));
  auto desc = iggpu::bind_group_desc(entries, layout);

  bind_group_ = device_.CreateBindGroup(&desc);
}
