#ifndef SANCTIFY_GAME_CLIENT_SRC_RENDER_TERRAIN_TERRAIN_PIPELINE_H
#define SANCTIFY_GAME_CLIENT_SRC_RENDER_TERRAIN_TERRAIN_PIPELINE_H

#include <igasset/proto/igasset.pb.h>
#include <igcore/either.h>
#include <iggpu/ubo_base.h>
#include <render/common/camera_ubo.h>
#include <render/terrain/terrain_geo.h>
#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp>

namespace sanctify::terrain_pipeline {

//
// TODO (sessamekesh): Rip out all this logic and replace it with the improved
// terrain code. This is all for very basic rendering.
//

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
};

// Pipeline inputs that are expected to change per-material (coloring params)
struct MaterialPipelineInputs {
  wgpu::BindGroup ObjectBindGroup;

  indigo::iggpu::UboBase<SolidColorParamsUboData> SolidColorParams;
};

//
// Pipeline data holder / utility struct
//
struct TerrainPipeline {
  wgpu::TextureFormat OutputFormat;
  wgpu::RenderPipeline Pipeline;

  FramePipelineInputs create_frame_inputs(
      const wgpu::Device& device,
      const render::CameraCommonVsUbo& camera_common_vs_ubo,
      const render::CameraCommonFsUbo& camera_common_fs_ubo) const;
  ScenePipelineInputs create_scene_inputs(
      const wgpu::Device& device,
      const render::CommonLightingUbo& common_lighting_ubo) const;
  MaterialPipelineInputs create_material_inputs(const wgpu::Device& device,
                                                glm::vec3 object_color) const;
};

// Render utility to catch missing bindings - should be used in a narrow scope
//  that does not outlive the RenderPassEncoder that commands are being recorded
//  to. Seeing this as a member variable would be a very bad thing.
class RenderUtil {
 public:
  RenderUtil(const wgpu::Device& device, const wgpu::RenderPassEncoder& pass,
             const TerrainPipeline& pipeline);

  RenderUtil& set_scene_inputs(const ScenePipelineInputs& inputs);
  RenderUtil& set_frame_inputs(const FramePipelineInputs& inputs);
  RenderUtil& set_material_inputs(const MaterialPipelineInputs& inputs);
  RenderUtil& set_geometry(const TerrainGeo& geo);
  RenderUtil& set_instances(const wgpu::Buffer& mat_world_instance_buffer,
                            int32_t num_instances);

  RenderUtil& draw();

 private:
  const wgpu::Device& device_;
  const wgpu::RenderPassEncoder& pass_;
  const TerrainPipeline& pipeline_;

  bool frame_inputs_set_;
  bool scene_inputs_set_;
  bool material_inputs_set_;

  bool vertex_buffer_set_;
  int32_t num_indices_;
  int32_t num_instances_;
};

class TerrainPipelineBuilder {
 public:
  static indigo::core::Maybe<TerrainPipelineBuilder> Create(
      const wgpu::Device& device, const indigo::asset::pb::WgslSource& vs_src,
      const indigo::asset::pb::WgslSource& fs_src);

  TerrainPipelineBuilder(wgpu::ShaderModule vert_module,
                         wgpu::ShaderModule frag_module,
                         std::string vs_entry_point,
                         std::string fs_entry_point);

  TerrainPipeline create_pipeline(const wgpu::Device& device,
                                  wgpu::TextureFormat swap_chain_format) const;

 private:
  wgpu::ShaderModule vert_module_;
  wgpu::ShaderModule frag_module_;

  std::string vs_entry_point_;
  std::string fs_entry_point_;
};

}  // namespace sanctify::terrain_pipeline

#endif
