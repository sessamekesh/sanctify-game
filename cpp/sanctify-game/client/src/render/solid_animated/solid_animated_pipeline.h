#ifndef SANCTIFY_GAME_CLIENT_SRC_RENDER_SOLID_ANIMATED_SOLID_ANIMATED_PIPELINE_H
#define SANCTIFY_GAME_CLIENT_SRC_RENDER_SOLID_ANIMATED_SOLID_ANIMATED_PIPELINE_H

#include <igasset/proto/igasset.pb.h>
#include <igcore/maybe.h>
#include <iggpu/thin_ubo.h>
#include <iggpu/ubo_base.h>
#include <render/common/camera_ubo.h>
#include <render/solid_animated/solid_animated_geo.h>
#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp>
#include <string>

namespace sanctify::solid_animated {

struct LightingParamsUboData {
  glm::vec3 LightDirection;
  float AmbientCoefficient;
  glm::vec3 LightColor;
  float SpecularPower;
};

struct SolidColorParamsUboData {
  glm::vec3 ObjectColor;
};

// Pipeline inputs that are expected to change per-frame (camera parameters)
struct FramePipelineInputs {
  wgpu::BindGroup FrameBindGroup;
};

// Pipeline inputs that are expected to change per-scene (lighting params)
struct ScenePipelineInputs {
  wgpu::BindGroup SceneBindGroup;

  indigo::iggpu::UboBase<LightingParamsUboData> LightingParamsUbo;
};

// Pipeline inputs that are expected to change per-material (coloring params)
struct MaterialPipelineInputs {
  wgpu::BindGroup ObjectBindGroup;

  indigo::iggpu::UboBase<SolidColorParamsUboData> SolidColorParams;
};

// Pipeline inputs that change per animation state (skeleton params)
struct AnimationPipelineInputs {
  wgpu::BindGroup AnimationBindGroup;

  indigo::iggpu::ThinUbo SkinMatricesUbo;
};

struct SolidAnimatedPipeline {
  static constexpr int kMaxBonesCount = 80;

  wgpu::TextureFormat OutputFormat;
  wgpu::RenderPipeline Pipeline;

  FramePipelineInputs create_frame_inputs(
      const wgpu::Device& device,
      const render::CameraCommonVsUbo& camera_vs_ubo,
      const render::CameraCommonFsUbo& camera_fs_ubo) const;
  ScenePipelineInputs create_scene_inputs(const wgpu::Device& device,
                                          glm::vec3 light_direction,
                                          glm::vec3 light_color,
                                          float ambient_coefficient,
                                          float specular_power) const;
  MaterialPipelineInputs create_material_inputs(const wgpu::Device& device,
                                                glm::vec3 object_color) const;
  AnimationPipelineInputs create_animation_inputs(
      const wgpu::Device& device) const;
};

class SolidAnimatedPipelineBuilder {
 public:
  static indigo::core::Maybe<SolidAnimatedPipelineBuilder> Create(
      const wgpu::Device& device,
      const indigo::asset::pb::WgslSource& vs_source,
      const indigo::asset::pb::WgslSource& fs_source);

  SolidAnimatedPipelineBuilder(wgpu::ShaderModule vert_module,
                               wgpu::ShaderModule frag_module,
                               std::string vs_entry_point,
                               std::string fs_entry_point);

  SolidAnimatedPipeline create_pipeline(
      const wgpu::Device& device, wgpu::TextureFormat swap_chain_format) const;

 private:
  wgpu::ShaderModule vert_module_;
  wgpu::ShaderModule frag_module_;

  std::string vs_entry_point_;
  std::string fs_entry_point_;
};

// Render utility to catch missing bindings and make the rendering process a
//  little bit more readable - should be used in a narrow scope so that it does
//  not outlive the RenderPassEncoder that commands are being recorded to.
class RenderUtil {
 public:
  RenderUtil(const wgpu::RenderPassEncoder& pass,
             const SolidAnimatedPipeline& pipeline);

  RenderUtil& set_scene_inputs(const ScenePipelineInputs& inputs);
  RenderUtil& set_frame_inputs(const FramePipelineInputs& inputs);
  RenderUtil& set_material_inputs(const MaterialPipelineInputs& inputs);
  RenderUtil& set_animation_inputs(const AnimationPipelineInputs& inputs);
  RenderUtil& set_geometry(const SolidAnimatedGeo& geo);
  RenderUtil& set_instances(const MatWorldInstanceBuffer& instances);
  RenderUtil& draw();

 private:
  const wgpu::RenderPassEncoder& pass_;
  const SolidAnimatedPipeline& pipeline_;

  bool frame_inputs_set_;
  bool scene_inputs_set_;
  bool material_inputs_set_;
  bool animation_inputs_set_;

  int32_t num_indices_;
  int32_t num_instances_;
};

}  // namespace sanctify::solid_animated

#endif
