#include <igcore/pod_vector.h>
#include <render/solid_animated/solid_animated_pipeline.h>

using namespace indigo;
using namespace iggpu;
using namespace sanctify;
using namespace solid_animated;

FramePipelineInputs SolidAnimatedPipeline::create_frame_inputs(
    const wgpu::Device& device, const render::CameraCommonVsUbo& camera_vs_ubo,
    const render::CameraCommonFsUbo& camera_fs_ubo) const {
  wgpu::BindGroupLayout per_frame_layout = Pipeline.GetBindGroupLayout(0);

  core::Vector<wgpu::BindGroupEntry> bind_group_entries(2);
  bind_group_entries.push_back(::buffer_bind_group_entry(
      0, camera_vs_ubo.buffer(), camera_vs_ubo.size()));
  bind_group_entries.push_back(::buffer_bind_group_entry(
      1, camera_fs_ubo.buffer(), camera_fs_ubo.size()));
  auto bind_group_desc =
      ::bind_group_desc(bind_group_entries, per_frame_layout,
                        "solid-animated-pipeline-frame-inputs");

  return FramePipelineInputs{device.CreateBindGroup(&bind_group_desc)};
}

ScenePipelineInputs SolidAnimatedPipeline::create_scene_inputs(
    const wgpu::Device& device, glm::vec3 light_direction,
    glm::vec3 light_color, float ambient_coefficient,
    float specular_power) const {
  wgpu::BindGroupLayout per_scene_layout = Pipeline.GetBindGroupLayout(2);

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
  auto bind_group_desc =
      ::bind_group_desc(bind_group_entries, per_scene_layout,
                        "solid-animated-pipeline-scene-inputs");

  wgpu::BindGroup bind_group = device.CreateBindGroup(&bind_group_desc);

  return ScenePipelineInputs{bind_group, std::move(lighting_params_ubo)};
}

MaterialPipelineInputs SolidAnimatedPipeline::create_material_inputs(
    const wgpu::Device& device, glm::vec3 object_color) const {
  wgpu::BindGroupLayout per_material_layout = Pipeline.GetBindGroupLayout(1);

  SolidColorParamsUboData material_params{};
  material_params.ObjectColor = object_color;
  UboBase<SolidColorParamsUboData> material_params_ubo(device, material_params);

  core::Vector<wgpu::BindGroupEntry> bind_group_entries(1);
  bind_group_entries.push_back(::buffer_bind_group_entry(
      0, material_params_ubo.buffer(), material_params_ubo.size()));
  auto bind_group_desc =
      ::bind_group_desc(bind_group_entries, per_material_layout,
                        "solid-animated-pipeline-material-inputs");

  wgpu::BindGroup bind_group = device.CreateBindGroup(&bind_group_desc);

  return MaterialPipelineInputs{bind_group, std::move(material_params_ubo)};
}

AnimationPipelineInputs SolidAnimatedPipeline::create_animation_inputs(
    const wgpu::Device& device) const {
  wgpu::BindGroupLayout per_skin_layout = Pipeline.GetBindGroupLayout(3);

  ThinUbo skin_matrices_ubo(sizeof(glm::mat4) * 80, device);

  core::Vector<wgpu::BindGroupEntry> bind_group_entries(1);
  bind_group_entries.push_back(::buffer_bind_group_entry(
      0, skin_matrices_ubo.buffer(), skin_matrices_ubo.size()));
  auto bind_group_desc =
      ::bind_group_desc(bind_group_entries, per_skin_layout,
                        "solid-animated-pipeline-animation-inputs");

  wgpu::BindGroup bind_group = device.CreateBindGroup(&bind_group_desc);

  return AnimationPipelineInputs{bind_group, std::move(skin_matrices_ubo)};
}

RenderUtil::RenderUtil(const wgpu::RenderPassEncoder& pass,
                       const SolidAnimatedPipeline& pipeline)
    : pass_(pass),
      pipeline_(pipeline),
      frame_inputs_set_(false),
      scene_inputs_set_(false),
      animation_inputs_set_(false),
      material_inputs_set_(false),
      num_indices_(-1),
      num_instances_(-1) {
  pass_.SetPipeline(pipeline.Pipeline);
}

RenderUtil& RenderUtil::set_scene_inputs(const ScenePipelineInputs& inputs) {
  pass_.SetBindGroup(2, inputs.SceneBindGroup);
  scene_inputs_set_ = true;
  return *this;
}

RenderUtil& RenderUtil::set_frame_inputs(const FramePipelineInputs& inputs) {
  pass_.SetBindGroup(0, inputs.FrameBindGroup);
  frame_inputs_set_ = true;
  return *this;
}

RenderUtil& RenderUtil::set_material_inputs(
    const MaterialPipelineInputs& inputs) {
  pass_.SetBindGroup(1, inputs.ObjectBindGroup);
  material_inputs_set_ = true;
  return *this;
}

RenderUtil& RenderUtil::set_animation_inputs(
    const AnimationPipelineInputs& inputs) {
  pass_.SetBindGroup(3, inputs.AnimationBindGroup);
  animation_inputs_set_ = true;
  return *this;
}

RenderUtil& RenderUtil::set_geometry(const SolidAnimatedGeo& geo) {
  pass_.SetVertexBuffer(0, geo.GeoVertexBuffer);
  pass_.SetVertexBuffer(2, geo.AnimationVertexBuffer);
  pass_.SetIndexBuffer(geo.IndexBuffer, geo.IndexFormat);
  num_indices_ = geo.NumIndices;
  return *this;
}

RenderUtil& RenderUtil::set_instances(
    const MatWorldInstanceBuffer& instance_buffer) {
  pass_.SetVertexBuffer(1, instance_buffer.InstanceBuffer);
  num_instances_ = instance_buffer.NumInstances;
  return *this;
}

RenderUtil& RenderUtil::draw() {
  if (frame_inputs_set_ && scene_inputs_set_ && material_inputs_set_ &&
      animation_inputs_set_ && num_indices_ > 0 && num_instances_ > 0) {
    pass_.DrawIndexed(num_indices_, num_instances_);
  }

  return *this;
}

core::Maybe<SolidAnimatedPipelineBuilder> SolidAnimatedPipelineBuilder::Create(
    const wgpu::Device& device, const asset::pb::WgslSource& vs_src,
    const asset::pb::WgslSource& fs_src) {
  return SolidAnimatedPipelineBuilder(
      ::create_shader_module(device, vs_src.shader_source()),
      ::create_shader_module(device, fs_src.shader_source()),
      vs_src.entry_point(), fs_src.entry_point());
}

SolidAnimatedPipelineBuilder::SolidAnimatedPipelineBuilder(
    wgpu::ShaderModule vert_module, wgpu::ShaderModule frag_module,
    std::string vs_entry_point, std::string fs_entry_point)
    : vert_module_(vert_module),
      frag_module_(frag_module),
      vs_entry_point_(vs_entry_point),
      fs_entry_point_(fs_entry_point) {}

SolidAnimatedPipeline SolidAnimatedPipelineBuilder::create_pipeline(
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

  core::PodVector<wgpu::VertexAttribute> geo_attributes(2);
  core::PodVector<wgpu::VertexAttribute> anim_attributes(2);
  core::PodVector<wgpu::VertexAttribute> mat_world_instance_attributes(4);

  geo_attributes.push_back(
      ::vertex_attribute(0, wgpu::VertexFormat::Float32x3, 0));
  geo_attributes.push_back(
      ::vertex_attribute(1, wgpu::VertexFormat::Float32x4, 12));

  wgpu::VertexBufferLayout geo_layout =
      ::vertex_buffer_layout(geo_attributes, 28, wgpu::VertexStepMode::Vertex);

  anim_attributes.push_back(
      ::vertex_attribute(6, wgpu::VertexFormat::Float32x4, 0));
  anim_attributes.push_back(
      ::vertex_attribute(7, wgpu::VertexFormat::Uint32x4, 16));

  wgpu::VertexBufferLayout anim_layout =
      ::vertex_buffer_layout(anim_attributes, 32, wgpu::VertexStepMode::Vertex);

  mat_world_instance_attributes.push_back(
      ::vertex_attribute(2, wgpu::VertexFormat::Float32x4, 0));
  mat_world_instance_attributes.push_back(
      ::vertex_attribute(3, wgpu::VertexFormat::Float32x4, 16));
  mat_world_instance_attributes.push_back(
      ::vertex_attribute(4, wgpu::VertexFormat::Float32x4, 32));
  mat_world_instance_attributes.push_back(
      ::vertex_attribute(5, wgpu::VertexFormat::Float32x4, 48));

  wgpu::VertexBufferLayout mat_world_layout = ::vertex_buffer_layout(
      mat_world_instance_attributes, 64, wgpu::VertexStepMode::Instance);

  wgpu::VertexBufferLayout vb_layouts[] = {geo_layout, mat_world_layout,
                                           anim_layout};

  wgpu::DepthStencilState depth_stencil_state =
      ::depth_stencil_state_standard();

  wgpu::RenderPipelineDescriptor desc{};
  desc.label = "SolidAnimatedPipeline";
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

  SolidAnimatedPipeline pipeline{};
  pipeline.OutputFormat = swap_chain_format;
  pipeline.Pipeline = gpu_pipeline;

  return pipeline;
}