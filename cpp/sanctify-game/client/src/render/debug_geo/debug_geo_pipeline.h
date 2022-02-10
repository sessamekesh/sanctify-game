#ifndef SANCTIFY_GAME_CLIENT_SRC_RENDER_DEBUG_GEO_DEBUG_GEO_PIPELINE_H
#define SANCTIFY_GAME_CLIENT_SRC_RENDER_DEBUG_GEO_DEBUG_GEO_PIPELINE_H

#include <igasset/proto/igasset.pb.h>
#include <igcore/maybe.h>
#include <iggpu/ubo_base.h>
#include <render/debug_geo/debug_geo.h>
#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp>

namespace sanctify::debug_geo {

struct CameraParamsUboData {
  glm::mat4 matView;
  glm::mat4 matProj;
};

struct LightingParamsUboData {
  glm::vec3 LightDirection;
  float AmbientCoefficient;
  glm::vec3 LightColor;
  float SpecularPower;
};

struct CameraFragmentParamsUboData {
  glm::vec3 cameraPos;
};

struct FramePipelineInputs {
  wgpu::BindGroup frameBindGroup;

  indigo::iggpu::UboBase<CameraParamsUboData> cameraVertParams;
  indigo::iggpu::UboBase<CameraFragmentParamsUboData> cameraFragParams;
};

struct ScenePipelineInputs {
  wgpu::BindGroup sceneBindGroup;

  indigo::iggpu::UboBase<LightingParamsUboData> lightingParams;
};

struct DebugGeoPipeline {
  wgpu::TextureFormat outputFormat;
  wgpu::RenderPipeline pipeline;

  FramePipelineInputs create_frame_inputs(const wgpu::Device& device) const;
  ScenePipelineInputs create_scene_inputs(const wgpu::Device& device,
                                          glm::vec3 light_direction,
                                          glm::vec3 light_color,
                                          float ambient_coefficient,
                                          float specular_power) const;
};

class DebugGeoPipelineBuilder {
 public:
  static indigo::core::Maybe<DebugGeoPipelineBuilder> Create(
      const wgpu::Device& device, const indigo::asset::pb::WgslSource& vs_src,
      const indigo::asset::pb::WgslSource& fs_src);

  DebugGeoPipelineBuilder(wgpu::ShaderModule vert_module,
                          wgpu::ShaderModule frag_module,
                          std::string vs_entry_point,
                          std::string fs_entry_point);

  DebugGeoPipeline create_pipeline(const wgpu::Device& device,
                                   wgpu::TextureFormat swap_chain_format) const;

 private:
  wgpu::ShaderModule vert_module_;
  wgpu::ShaderModule frag_module_;

  std::string vs_entry_point_;
  std::string fs_entry_point_;
};

// Render utility to catch missing bindings and make rendering a bit more
//  readable - should be used in a narrow scope and not outlive
//  RenderPassEncoder that is sent to it in ctor
class RenderUtil {
 public:
  RenderUtil(const wgpu::RenderPassEncoder& pass,
             const DebugGeoPipeline& pipeline);

  RenderUtil& set_scene_inputs(const ScenePipelineInputs& inputs);
  RenderUtil& set_frame_inputs(const FramePipelineInputs& inputs);
  RenderUtil& set_geometry(const DebugGeo& geo);
  RenderUtil& set_instances(const InstanceBuffer& instances);
  RenderUtil& draw();

 private:
  const wgpu::RenderPassEncoder& pass_;
  const DebugGeoPipeline& pipeline_;

  bool frame_inputs_set_;
  bool scene_inputs_set_;

  int32_t num_indices_;
  int32_t num_instances_;
};

}  // namespace sanctify::debug_geo

#endif
