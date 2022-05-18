#include "pve_offline_render.h"

#include <common/render/common/render_components.h>
#include <common/render/solid_static/ecs_util.h>
#include <common/render/solid_static/pipeline.h>
#include <common/render/tonemap/ecs_util.h>
#include <common/render/tonemap/pipeline.h>

using namespace sanctify;
using namespace pve;

using namespace indigo;

namespace {

const float kAvgLuminosity = 0.2f;

struct CtxDepthStencil {
  iggpu::Texture depthTexture;
  wgpu::TextureView depthView;
};

struct CtxHdrBuffer {
  iggpu::Texture texture;
  wgpu::TextureView view;
};

struct CtxSolidGeoInputs {
  render::solid_static::SceneInputs sceneInputs;
  render::solid_static::FrameInputs frameInputs;
};

struct CtxTonemappingInputs {
  bool is_dirty;
  render::tonemap::TonemappingArgsInputs tonemappingInputs;
  render::tonemap::HdrTextureInputs hdrTextureInputs;
};

wgpu::TextureView get_depth_view(
    const render::CtxPlatformObjects& platform_objects, igecs::WorldView* wv) {
  if (!wv->ctx_has<CtxDepthStencil>() ||
      wv->ctx<CtxDepthStencil>().depthTexture.Width !=
          platform_objects.viewportWidth ||
      wv->ctx<CtxDepthStencil>().depthTexture.Height !=
          platform_objects.viewportHeight) {
    iggpu::Texture depth_texture = iggpu::create_empty_texture_2d(
        platform_objects.device, platform_objects.viewportWidth,
        platform_objects.viewportHeight, 1,
        wgpu::TextureFormat::Depth24PlusStencil8,
        wgpu::TextureUsage::RenderAttachment);

    auto depth_view_desc = iggpu::view_desc_of(depth_texture);
    wgpu::TextureView depth_view =
        depth_texture.GpuTexture.CreateView(&depth_view_desc);
    wv->attach_ctx<CtxDepthStencil>(std::move(depth_texture),
                                    std::move(depth_view));
  }

  return wv->ctx<CtxDepthStencil>().depthView;
}

wgpu::TextureView get_hdr_buffer(
    const render::CtxPlatformObjects& platform_objects,
    const render::CtxHdrFramebufferParams& hdr_params, igecs::WorldView* wv) {
  if (!wv->ctx_has<CtxHdrBuffer>() ||
      wv->ctx<CtxHdrBuffer>().texture.Width != hdr_params.width ||
      wv->ctx<CtxHdrBuffer>().texture.Height != hdr_params.height ||
      wv->ctx<CtxHdrBuffer>().texture.Format != hdr_params.format) {
    iggpu::Texture hdr_tex = iggpu::create_empty_texture_2d(
        platform_objects.device, hdr_params.width, hdr_params.height, 1,
        hdr_params.format,
        wgpu::TextureUsage::RenderAttachment |
            wgpu::TextureUsage::TextureBinding);

    auto view_desc = iggpu::view_desc_of(hdr_tex);
    wgpu::TextureView hdr_view = hdr_tex.GpuTexture.CreateView(&view_desc);
    wv->attach_ctx<CtxHdrBuffer>(std::move(hdr_tex), std::move(hdr_view));

    if (wv->ctx_has<CtxTonemappingInputs>()) {
      wv->mut_ctx<CtxTonemappingInputs>().is_dirty = true;
    }
  }

  return wv->ctx<CtxHdrBuffer>().view;
}

CtxSolidGeoInputs& get_solid_geo_inputs(
    const wgpu::Device& device,
    const render::CtxMainCameraCommonUbos& camera_ubos,
    const render::solid_static::Pipeline& pipeline, igecs::WorldView* wv) {
  if (!wv->ctx_has<CtxSolidGeoInputs>()) {
    wv->attach_ctx<CtxSolidGeoInputs>(
        pipeline.create_scene_inputs(device, camera_ubos.lightingUbo),
        pipeline.create_frame_inputs(device, camera_ubos.cameraVsUbo,
                                     camera_ubos.cameraFsUbo));
  }

  return wv->mut_ctx<CtxSolidGeoInputs>();
}

CtxTonemappingInputs& get_tonemapping_inputs(
    const render::CtxPlatformObjects& platform_objects,
    const render::CtxHdrFramebufferParams& hdr_params,
    const render::tonemap::Pipeline& pipeline, igecs::WorldView* wv) {
  if (!wv->ctx_has<CtxTonemappingInputs>() ||
      wv->ctx<CtxTonemappingInputs>().is_dirty) {
    wv->attach_ctx<CtxTonemappingInputs>(
        false,
        pipeline.create_tonemapping_args_inputs(
            platform_objects.device,
            render::tonemap::TonemappingArgumentsData{::kAvgLuminosity}),
        pipeline.create_hdr_texture_inputs(
            platform_objects.device,
            ::get_hdr_buffer(platform_objects, hdr_params, wv)));
  }

  return wv->mut_ctx<CtxTonemappingInputs>();
}

}  // namespace

const igecs::WorldView::Decl& PveOfflineRenderSystem::render_decl() {
  static const igecs::WorldView::Decl kRenderDecl =
      igecs::WorldView::Decl()
          .ctx_reads<render::CtxHdrFramebufferParams>()
          .ctx_reads<render::CtxPlatformObjects>()
          .ctx_writes<CtxDepthStencil>()
          .ctx_writes<CtxHdrBuffer>()
          .ctx_writes<render::solid_static::CtxPipeline>()
          .ctx_reads<render::CtxMainCameraCommonUbos>()
          .ctx_writes<CtxSolidGeoInputs>()
          .ctx_writes<render::tonemap::CtxPipeline>()
          .ctx_writes<CtxTonemappingInputs>()
          .merge_in_decl(render::solid_static::EcsUtil::render_decl());
  return kRenderDecl;
}

void PveOfflineRenderSystem::render(igecs::WorldView* wv) {
  const auto& ctx_platform = wv->ctx<render::CtxPlatformObjects>();
  const auto& ctx_hdr_params = wv->ctx<render::CtxHdrFramebufferParams>();
  const auto& ctx_main_camera_ubos = wv->ctx<render::CtxMainCameraCommonUbos>();

  const auto& device = ctx_platform.device;

  wgpu::CommandEncoder main_pass_command_encoder =
      device.CreateCommandEncoder();

  render::solid_static::EcsUtil::update_pipeline(wv, device);
  render::tonemap::EcsUtil::update_pipeline(wv);

  //
  // HDR GEO PASS
  //
  {
    wgpu::RenderPassDepthStencilAttachment depth_attachment{};
    depth_attachment.depthClearValue = 1.f;
    depth_attachment.stencilClearValue = 0x00;
    depth_attachment.depthLoadOp = wgpu::LoadOp::Clear;
    depth_attachment.depthStoreOp = wgpu::StoreOp::Store;
    depth_attachment.stencilLoadOp = wgpu::LoadOp::Clear;
    depth_attachment.stencilStoreOp = wgpu::StoreOp::Discard;
    depth_attachment.view = ::get_depth_view(ctx_platform, wv);

    static float t = 0.1f;
    t += 0.1f;

    wgpu::RenderPassColorAttachment color_attachment{};
    color_attachment.clearValue = {glm::cos(t), 0.1f, 0.1f, 1.f};
    color_attachment.loadOp = wgpu::LoadOp::Clear;
    color_attachment.storeOp = wgpu::StoreOp::Store;
    color_attachment.view = ::get_hdr_buffer(ctx_platform, ctx_hdr_params, wv);

    wgpu::RenderPassDescriptor hdr_geo_pass_desc{};
    hdr_geo_pass_desc.colorAttachments = &color_attachment;
    hdr_geo_pass_desc.colorAttachmentCount = 1;
    hdr_geo_pass_desc.depthStencilAttachment = &depth_attachment;

    auto pass = main_pass_command_encoder.BeginRenderPass(&hdr_geo_pass_desc);
    const auto& pipeline = render::solid_static::EcsUtil::get_pipeline(wv);

    auto& inputs =
        ::get_solid_geo_inputs(device, ctx_main_camera_ubos, pipeline, wv);

    sanctify::render::solid_static::RenderUtil util(&pass, &pipeline);
    util.set_frame_inputs(inputs.frameInputs)
        .set_scene_inputs(inputs.sceneInputs);

    sanctify::render::solid_static::EcsUtil::render_matching(
        wv, ctx_platform.device, &util, [](auto) { return true; });

    pass.End();
  }

  //
  // TONEMAPPING PASS
  //
  {
    wgpu::RenderPassColorAttachment color_attachment{};
    color_attachment.clearValue = {0.f, 0.f, 0.f, 1.f};
    color_attachment.loadOp = wgpu::LoadOp::Clear;
    color_attachment.storeOp = wgpu::StoreOp::Store;
    color_attachment.view = ctx_platform.swapChainBackbuffer;

    wgpu::RenderPassDescriptor ldr_tonemap_pass_desc{};
    ldr_tonemap_pass_desc.colorAttachmentCount = 1;
    ldr_tonemap_pass_desc.colorAttachments = &color_attachment;

    auto pass =
        main_pass_command_encoder.BeginRenderPass(&ldr_tonemap_pass_desc);
    const auto& pipeline =
        wv->ctx<render::tonemap::CtxPipeline>().currentPipeline;

    auto& inputs =
        ::get_tonemapping_inputs(ctx_platform, ctx_hdr_params, pipeline, wv);

    render::tonemap::RenderUtil util(&pass, &pipeline);

    bool is_success = false;
    util.set_hdr_texture_inputs(inputs.hdrTextureInputs)
        .set_tonemapping_args_inputs(inputs.tonemappingInputs)
        .draw(&is_success);

    pass.End();
  }

  auto commands = main_pass_command_encoder.Finish();
  device.GetQueue().Submit(1, &commands);
}
