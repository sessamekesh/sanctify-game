#include <igcore/vector.h>
#include <render/debug_geo/debug_geo_pipeline.h>

using namespace sanctify;
using namespace debug_geo;

using namespace indigo;
using namespace core;
using namespace iggpu;

FramePipelineInputs DebugGeoPipeline::create_frame_inputs(
    const wgpu::Device& device) const {
  wgpu::BindGroupLayout per_frame_layout = pipeline.GetBindGroupLayout(0);

  UboBase<CameraParamsUboData> vert_params_ubo(device);
  UboBase<CameraFragmentParamsUboData> frag_params_ubo(device);

  Vector<wgpu::BindGroupEntry> bind_group_entries(2);
  bind_group_entries.push_back(::buffer_bind_group_entry(
      0, vert_params_ubo.buffer(), vert_params_ubo.size()));
  bind_group_entries.push_back(::buffer_bind_group_entry(
      1, frag_params_ubo.buffer(), frag_params_ubo.size()));

  auto bind_group_desc = ::bind_group_desc(bind_group_entries, per_frame_layout,
                                           "debug-geo-frame-inputs-bind-group");

  wgpu::BindGroup bind_group = device.CreateBindGroup(&bind_group_desc);

  return FramePipelineInputs{bind_group, std::move(vert_params_ubo),
                             std::move(frag_params_ubo)};
}

ScenePipelineInputs DebugGeoPipeline::create_scene_inputs(
    const wgpu::Device& device, glm::vec3 light_direction,
    glm::vec3 light_color, float ambient_coefficient,
    float specular_power) const {
  wgpu::BindGroupLayout per_scene_layout = pipeline.GetBindGroupLayout(1);

  LightingParamsUboData lighting_params_data{};
  lighting_params_data.AmbientCoefficient = ambient_coefficient;
  lighting_params_data.LightDirection = light_direction;
  lighting_params_data.SpecularPower = specular_power;
  lighting_params_data.LightColor = light_color;
  UboBase<LightingParamsUboData> lighting_params_ubo(device,
                                                     lighting_params_data);

  core::Vector<wgpu::BindGroupEntry> bind_group_entries(1);
  bind_group_entries.push_back(::buffer_bind_group_entry(
      0, lighting_params_ubo.buffer(), lighting_params_ubo.size()));
  auto bind_group_desc = ::bind_group_desc(bind_group_entries, per_scene_layout,
                                           "debug-geo-scene-inputs");

  wgpu::BindGroup bind_group = device.CreateBindGroup(&bind_group_desc);

  return ScenePipelineInputs{bind_group, std::move(lighting_params_ubo)};
}

Maybe<DebugGeoPipelineBuilder> DebugGeoPipelineBuilder::Create(
    const wgpu::Device& device, const indigo::asset::pb::WgslSource& vs_src,
    const indigo::asset::pb::WgslSource& fs_src) {
  return DebugGeoPipelineBuilder(
      ::create_shader_module(device, vs_src.shader_source()),
      ::create_shader_module(device, fs_src.shader_source()),
      vs_src.entry_point(), fs_src.entry_point());
}

DebugGeoPipelineBuilder::DebugGeoPipelineBuilder(wgpu::ShaderModule vert_module,
                                                 wgpu::ShaderModule frag_module,
                                                 std::string vs_entry_point,
                                                 std::string fs_entry_point)
    : vert_module_(vert_module),
      frag_module_(frag_module),
      vs_entry_point_(vs_entry_point),
      fs_entry_point_(fs_entry_point) {}

DebugGeoPipeline DebugGeoPipelineBuilder::create_pipeline(
    const wgpu::Device& device, wgpu::TextureFormat swap_chain_format) const {
  wgpu::ColorTargetState color_target_state{};
  color_target_state.format = swap_chain_format;

  wgpu::FragmentState fragment_state{};
  fragment_state.module = frag_module_;
  fragment_state.entryPoint = fs_entry_point_.c_str();
  fragment_state.constantCount = 0;
  fragment_state.constants = nullptr;
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target_state;

  PodVector<wgpu::VertexAttribute> geo_attributes(2);
  PodVector<wgpu::VertexAttribute> instance_attributes(5);

  geo_attributes.push_back(::vertex_attribute(
      0, wgpu::VertexFormat::Float32x3,
      offsetof(asset::LegacyPositionNormalVertexData, Position)));
  geo_attributes.push_back(::vertex_attribute(
      1, wgpu::VertexFormat::Float32x3,
      offsetof(asset::LegacyPositionNormalVertexData, Normal)));

  wgpu::VertexBufferLayout geo_layout = ::vertex_buffer_layout(
      geo_attributes, sizeof(asset::LegacyPositionNormalVertexData),
      wgpu::VertexStepMode::Vertex);

  instance_attributes.push_back(::vertex_attribute(
      2, wgpu::VertexFormat::Float32x4, offsetof(InstanceData, MatWorld) + 0));
  instance_attributes.push_back(
      ::vertex_attribute(3, wgpu::VertexFormat::Float32x4,
                         offsetof(InstanceData, MatWorld) + sizeof(glm::vec4)));
  instance_attributes.push_back(::vertex_attribute(
      4, wgpu::VertexFormat::Float32x4,
      offsetof(InstanceData, MatWorld) + 2 * sizeof(glm::vec4)));
  instance_attributes.push_back(::vertex_attribute(
      5, wgpu::VertexFormat::Float32x4,
      offsetof(InstanceData, MatWorld) + 3 * sizeof(glm::vec4)));
  instance_attributes.push_back(::vertex_attribute(
      6, wgpu::VertexFormat::Float32x3, offsetof(InstanceData, ObjectColor)));

  wgpu::VertexBufferLayout instance_layout =
      ::vertex_buffer_layout(instance_attributes, sizeof(InstanceData),
                             wgpu::VertexStepMode::Instance);

  wgpu::VertexBufferLayout vb_layouts[] = {geo_layout, instance_layout};

  wgpu::DepthStencilState depth_stencil_state =
      ::depth_stencil_state_standard();

  wgpu::RenderPipelineDescriptor desc{};
  desc.label = "DebugGeoPipeline";
  desc.vertex.buffers = vb_layouts;
  desc.vertex.bufferCount = std::size(vb_layouts);
  desc.vertex.module = vert_module_;
  desc.vertex.entryPoint = vs_entry_point_.c_str();
  desc.fragment = &fragment_state;
  desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  desc.depthStencil = &depth_stencil_state;
  desc.primitive.cullMode = wgpu::CullMode::Back;
  desc.primitive.frontFace = wgpu::FrontFace::CCW;

  auto gpu_pipeline = device.CreateRenderPipeline(&desc);

  DebugGeoPipeline pipeline{};
  pipeline.outputFormat = swap_chain_format;
  pipeline.pipeline = gpu_pipeline;

  return pipeline;
}

RenderUtil::RenderUtil(const wgpu::RenderPassEncoder& pass,
                       const DebugGeoPipeline& pipeline)
    : pass_(pass),
      pipeline_(pipeline),
      frame_inputs_set_(false),
      scene_inputs_set_(false),
      num_indices_(-1),
      num_instances_(-1) {
  pass_.SetPipeline(pipeline.pipeline);
}

RenderUtil& RenderUtil::set_scene_inputs(const ScenePipelineInputs& inputs) {
  pass_.SetBindGroup(1, inputs.sceneBindGroup);
  scene_inputs_set_ = true;
  return *this;
}

RenderUtil& RenderUtil::set_frame_inputs(const FramePipelineInputs& inputs) {
  pass_.SetBindGroup(0, inputs.frameBindGroup);
  frame_inputs_set_ = true;
  return *this;
}

RenderUtil& RenderUtil::set_geometry(const DebugGeo& geo) {
  pass_.SetVertexBuffer(0, geo.GeoVertexBuffer);
  pass_.SetIndexBuffer(geo.IndexBuffer, geo.IndexFormat);
  num_indices_ = geo.NumIndices;
  return *this;
}

RenderUtil& RenderUtil::set_instances(const InstanceBuffer& instance_buffer) {
  pass_.SetVertexBuffer(1, instance_buffer.instanceBuffer);
  num_instances_ = instance_buffer.numInstances;
  return *this;
}

RenderUtil& RenderUtil::draw() {
  if (frame_inputs_set_ && scene_inputs_set_ && num_indices_ > 0 &&
      num_instances_ > 0) {
    pass_.DrawIndexed(num_indices_, num_instances_);
  }

  return *this;
}
