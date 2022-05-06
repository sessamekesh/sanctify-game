#include "pipeline.h"

#include <igcore/vector.h>
#include <webgpu/webgpu_cpp.h>

using namespace sanctify;
using namespace render;
using namespace tonemap;
using namespace indigo;
using namespace core;

TonemappingArgsInputs Pipeline::create_tonemapping_args_inputs(
    const wgpu::Device& device, TonemappingArgumentsData data) const {
  wgpu::BindGroupLayout layout = pipeline.GetBindGroupLayout(0);

  iggpu::UboBase<TonemappingArgumentsData> args_ubo(device, data);

  // Create the cheapest, simplest possible sampler because the output
  // should be 1:1 with the input (this is a texture filter!)
  wgpu::SamplerDescriptor sampler_desc{};
  sampler_desc.addressModeU = wgpu::AddressMode::ClampToEdge;
  sampler_desc.addressModeV = wgpu::AddressMode::ClampToEdge;
  sampler_desc.addressModeW = wgpu::AddressMode::ClampToEdge;
  sampler_desc.label = "tonemapping-pipeline-sampler";
  sampler_desc.lodMaxClamp = 0;
  sampler_desc.lodMinClamp = 0;
  sampler_desc.magFilter = wgpu::FilterMode::Nearest;
  sampler_desc.maxAnisotropy = 1;
  sampler_desc.minFilter = wgpu::FilterMode::Nearest;
  sampler_desc.mipmapFilter = wgpu::FilterMode::Nearest;

  wgpu::Sampler tonemap_sampler = device.CreateSampler(&sampler_desc);

  Vector<wgpu::BindGroupEntry> entries(2);
  entries.push_back(
      iggpu::buffer_bind_group_entry(0, args_ubo.buffer(), args_ubo.size()));
  entries.push_back(iggpu::sampler_bind_group_entry(1, tonemap_sampler));

  auto desc = iggpu::bind_group_desc(entries, layout);

  return TonemappingArgsInputs{std::move(args_ubo), tonemap_sampler,
                               device.CreateBindGroup(&desc)};
}

HdrTextureInputs Pipeline::create_hdr_texture_inputs(
    const wgpu::Device& device, wgpu::TextureView hdr_input_view) const {
  wgpu::BindGroupLayout layout = pipeline.GetBindGroupLayout(1);

  Vector<wgpu::BindGroupEntry> entries(1);
  entries.push_back(iggpu::tex_view_bind_group_entry(0, hdr_input_view));
  wgpu::BindGroupDescriptor desc = iggpu::bind_group_desc(entries, layout);

  return HdrTextureInputs{device.CreateBindGroup(&desc)};
}

Pipeline Pipeline::FromBuilder(const wgpu::Device& device,
                               const iggpu::PipelineBuilder& pipeline_builder,
                               wgpu::TextureFormat output_format) {
  wgpu::ColorTargetState color_target_state{};
  color_target_state.format = output_format;

  wgpu::FragmentState fragment_state = iggpu::standard_fragment_state(
      color_target_state, pipeline_builder.fragModule,
      pipeline_builder.fsEntryPoint.c_str());

  wgpu::RenderPipelineDescriptor desc{};
  desc.label = "TonemapPipeline";
  desc.vertex.module = pipeline_builder.vertModule;
  desc.vertex.entryPoint = pipeline_builder.vsEntryPoint.c_str();
  desc.fragment = &fragment_state;
  desc.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;
  desc.primitive.stripIndexFormat = wgpu::IndexFormat::Uint16;
  desc.primitive.cullMode = wgpu::CullMode::None;
  desc.primitive.frontFace = wgpu::FrontFace::CCW;

  auto gpu_pipeline = device.CreateRenderPipeline(&desc);

  return Pipeline{output_format, gpu_pipeline};
}

RenderUtil::RenderUtil(const wgpu::RenderPassEncoder* pass,
                       const Pipeline* pipeline)
    : pass_(pass),
      pipeline_(pipeline),
      tonemapping_args_set_(false),
      texture_inputs_set_(false) {
  pass->SetPipeline(pipeline->pipeline);
}

RenderUtil& RenderUtil::set_tonemapping_args_inputs(
    const TonemappingArgsInputs& inputs) {
  tonemapping_args_set_ = true;
  pass_->SetBindGroup(0, inputs.bindGroup);
  return *this;
}

RenderUtil& RenderUtil::set_hdr_texture_inputs(const HdrTextureInputs& inputs) {
  texture_inputs_set_ = true;
  pass_->SetBindGroup(1, inputs.bindGroup);
  return *this;
}

RenderUtil& RenderUtil::draw(bool* o_success) {
  static bool dummyBool = false;
  if (!o_success) o_success = &dummyBool;

  if (tonemapping_args_set_ && texture_inputs_set_) {
    pass_->Draw(4);
    *o_success = true;
    return *this;
  }

  *o_success = false;
  return *this;
}
