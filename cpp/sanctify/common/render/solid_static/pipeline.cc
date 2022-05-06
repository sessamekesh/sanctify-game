#include "pipeline.h"

#include <igcore/vector.h>
#include <iggpu/util.h>

using namespace sanctify;
using namespace render;
using namespace solid_static;

using namespace indigo;
using namespace core;

FrameInputs Pipeline::create_frame_inputs(
    const wgpu::Device& device, const CameraCommonVsUbo& camera_common_vs_ubo,
    const CameraCommonFsUbo& camera_common_fs_ubo) const {
  wgpu::BindGroupLayout per_frame_layout = pipeline.GetBindGroupLayout(0);

  Vector<wgpu::BindGroupEntry> bind_group_entries(2);
  bind_group_entries.push_back(iggpu::buffer_bind_group_entry(
      0, camera_common_vs_ubo.buffer(), camera_common_vs_ubo.size()));
  bind_group_entries.push_back(iggpu::buffer_bind_group_entry(
      1, camera_common_fs_ubo.buffer(), camera_common_fs_ubo.size()));

  auto bind_group_desc = iggpu::bind_group_desc(
      bind_group_entries, per_frame_layout, "sold_static-frame-inputs-bg");
  wgpu::BindGroup bind_group = device.CreateBindGroup(&bind_group_desc);

  return FrameInputs{bind_group};
}

SceneInputs Pipeline::create_scene_inputs(
    const wgpu::Device& device,
    const render::CommonLightingUbo& common_lighting_ubo) const {
  wgpu::BindGroupLayout per_scene_layout = pipeline.GetBindGroupLayout(1);

  core::Vector<wgpu::BindGroupEntry> bind_group_entries(1);
  bind_group_entries.push_back(iggpu::buffer_bind_group_entry(
      0, common_lighting_ubo.buffer(), common_lighting_ubo.size()));
  auto bind_group_desc = iggpu::bind_group_desc(
      bind_group_entries, per_scene_layout, "sold_static-scene-inputs-bg");

  wgpu::BindGroup bind_group = device.CreateBindGroup(&bind_group_desc);

  return SceneInputs{bind_group};
}

PipelineBuilder PipelineBuilder::Create(
    const wgpu::Device& device, const indigo::asset::pb::WgslSource& vs_src,
    const indigo::asset::pb::WgslSource& fs_src) {
  return PipelineBuilder(
      iggpu::create_shader_module(device, vs_src.shader_source()),
      iggpu::create_shader_module(device, fs_src.shader_source()),
      vs_src.entry_point(), fs_src.entry_point());
}

Pipeline PipelineBuilder::create_pipeline(
    const wgpu::Device& device, wgpu::TextureFormat output_format) const {
  wgpu::ColorTargetState color_target_state{};
  color_target_state.format = output_format;

  wgpu::FragmentState fragment_state{};
  fragment_state.module = frag_module_;
  fragment_state.entryPoint = fs_entry_point_.c_str();
  fragment_state.constantCount = 0;
  fragment_state.constants = nullptr;
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target_state;

  PodVector<wgpu::VertexAttribute> geo_attributes(2);
  PodVector<wgpu::VertexAttribute> instance_attributes(7);

  // Per-vertex attributes

  // @location(0) position
  geo_attributes.push_back(iggpu::vertex_attribute(
      0, wgpu::VertexFormat::Float32x3,
      offsetof(asset::PositionNormalVertexData, Position)));
  // @location(1) tbn_quat
  geo_attributes.push_back(iggpu::vertex_attribute(
      1, wgpu::VertexFormat::Float32x4,
      offsetof(asset::PositionNormalVertexData, NormalQuat)));

  wgpu::VertexBufferLayout geo_layout = iggpu::vertex_buffer_layout(
      geo_attributes, sizeof(asset::PositionNormalVertexData),
      wgpu::VertexStepMode::Vertex);

  // Per-instance attributes

  // @location(2-4) mat_world_0-3
  instance_attributes.push_back(iggpu::vertex_attribute(
      2, wgpu::VertexFormat::Float32x4, offsetof(InstanceData, matWorld)));
  instance_attributes.push_back(iggpu::vertex_attribute(
      3, wgpu::VertexFormat::Float32x4,
      offsetof(InstanceData, matWorld) + sizeof(glm::vec4)));
  instance_attributes.push_back(iggpu::vertex_attribute(
      4, wgpu::VertexFormat::Float32x4,
      offsetof(InstanceData, matWorld) + sizeof(glm::vec4) * 2));
  // @location(5) albedo
  instance_attributes.push_back(iggpu::vertex_attribute(
      5, wgpu::VertexFormat::Float32, offsetof(InstanceData, albedo)));
  // @location(6) metallic
  instance_attributes.push_back(iggpu::vertex_attribute(
      6, wgpu::VertexFormat::Float32, offsetof(InstanceData, metallic)));
  // @location(7) roughness
  instance_attributes.push_back(iggpu::vertex_attribute(
      7, wgpu::VertexFormat::Float32, offsetof(InstanceData, roughness)));
  // @location(8) ao
  instance_attributes.push_back(
      iggpu::vertex_attribute(8, wgpu::VertexFormat::Float32,
                              offsetof(InstanceData, ambientOcclusion)));

  wgpu::VertexBufferLayout instance_layout =
      iggpu::vertex_buffer_layout(instance_attributes, sizeof(InstanceData),
                                  wgpu::VertexStepMode::Instance);

  wgpu::VertexBufferLayout vb_layouts[] = {geo_layout, instance_layout};

  wgpu::DepthStencilState depth_stencil_state =
      iggpu::depth_stencil_state_standard();

  wgpu::RenderPipelineDescriptor desc{};
  desc.label = "SolidStaticPipeline";
  desc.vertex.bufferCount = std::size(vb_layouts);
  desc.vertex.buffers = vb_layouts;
  desc.vertex.module = vert_module_;
  desc.vertex.entryPoint = vs_entry_point_.c_str();
  desc.fragment = &fragment_state;
  desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  desc.depthStencil = &depth_stencil_state;
  desc.primitive.cullMode = wgpu::CullMode::Back;
  desc.primitive.frontFace = wgpu::FrontFace::CCW;

  auto gpu_pipeline = device.CreateRenderPipeline(&desc);

  return Pipeline{output_format, gpu_pipeline};
}

PipelineBuilder::PipelineBuilder(wgpu::ShaderModule vert_module,
                                 wgpu::ShaderModule frag_module,
                                 std::string vs_entry_point,
                                 std::string fs_entry_point)
    : vert_module_(vert_module),
      frag_module_(frag_module),
      vs_entry_point_(vs_entry_point),
      fs_entry_point_(fs_entry_point) {}

RenderUtil::RenderUtil(const wgpu::RenderPassEncoder* pass,
                       const Pipeline* pipeline)
    : pass_(pass),
      pipeline_(pipeline),
      frame_inputs_set_(false),
      scene_inputs_set_(false),
      num_indices_(-1),
      num_instances_(-1) {
  pass->SetPipeline(pipeline->pipeline);
}

RenderUtil& RenderUtil::set_scene_inputs(const SceneInputs& inputs) {
  pass_->SetBindGroup(1, inputs.sceneBindGroup);
  scene_inputs_set_ = true;
  return *this;
}

RenderUtil& RenderUtil::set_frame_inputs(const FrameInputs& input) {
  pass_->SetBindGroup(0, input.frameBindGroup);
  frame_inputs_set_ = true;
  return *this;
}

RenderUtil& RenderUtil::set_geometry(const Geo& geo) {
  pass_->SetVertexBuffer(0, geo.geoVertexBuffer);
  pass_->SetIndexBuffer(geo.indexBuffer, geo.indexFormat);
  num_indices_ = geo.numIndices;
  return *this;
}

RenderUtil& RenderUtil::set_instances(const wgpu::Buffer& buffer,
                                      uint32_t num_instances) {
  pass_->SetVertexBuffer(1, buffer);
  num_instances_ = num_instances;
  return *this;
}

RenderUtil& RenderUtil::draw(bool* o_success) {
  if (frame_inputs_set_ && scene_inputs_set_ && num_indices_ > 0 &&
      num_instances_ > 0) {
    pass_->DrawIndexed(num_indices_, num_instances_);
    if (o_success) *o_success = true;
    return *this;
  }

  if (o_success) *o_success = false;

  return *this;
}
