#ifndef SANCTIFY_COMMON_RENDER_TONEMAP_TONEMAP_PIPELINE_H
#define SANCTIFY_COMMON_RENDER_TONEMAP_TONEMAP_PIPELINE_H

#include <igasset/proto/igasset.pb.h>
#include <iggpu/pipeline_builder.h>
#include <iggpu/texture.h>
#include <iggpu/ubo_base.h>

#include <string>

namespace sanctify::render::tonemap {

struct TonemappingArgumentsData {
  float averageLuminosity;
};

struct TonemappingArgsInputs {
  indigo::iggpu::UboBase<TonemappingArgumentsData> ubo;
  wgpu::Sampler sampler;

  wgpu::BindGroup bindGroup;
};

struct HdrTextureInputs {
  wgpu::BindGroup bindGroup;
};

struct Pipeline {
  wgpu::TextureFormat outputFormat;
  wgpu::RenderPipeline pipeline;

  static Pipeline FromBuilder(
      const wgpu::Device& device,
      const indigo::iggpu::PipelineBuilder& pipeline_builder,
      wgpu::TextureFormat output_format);

  TonemappingArgsInputs create_tonemapping_args_inputs(
      const wgpu::Device& device,
      TonemappingArgumentsData tonemapping_args) const;
  HdrTextureInputs create_hdr_texture_inputs(
      const wgpu::Device& device, wgpu::TextureView hdr_input_view) const;
};

// Utility to submit draw calls, and make rendering code somewhat readable.
//  Should be kept in a pretty narrow scope and obviously not outlive the
//  RenderPassEncoder that is sent into the ctor (no heap alloc on this!)
class RenderUtil {
 public:
  RenderUtil(const wgpu::RenderPassEncoder* pass, const Pipeline* pipeline);

  RenderUtil& set_tonemapping_args_inputs(const TonemappingArgsInputs& inputs);
  RenderUtil& set_hdr_texture_inputs(const HdrTextureInputs& inputs);

  RenderUtil& draw(bool* o_success = nullptr);

 private:
  // Do not define to get a nice static guarantee that this class is never
  // allocated on the heap
  void* operator new(size_t size);

  const wgpu::RenderPassEncoder* pass_;
  const Pipeline* pipeline_;

  bool tonemapping_args_set_;
  bool texture_inputs_set_;
};

}  // namespace sanctify::render::tonemap

#endif
