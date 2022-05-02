#ifndef SANCTIFY_COMMON_RENDER_SOLID_STATIC_PIPELINE_H
#define SANCTIFY_COMMON_RENDER_SOLID_STATIC_PIPELINE_H

#include <common/render/common/camera_ubos.h>
#include <igasset/proto/igasset.pb.h>
#include <igasync/promise.h>
#include <igcore/maybe.h>
#include <webgpu/webgpu_cpp.h>

#include "solid_static_geo.h"

namespace sanctify::render::solid_static {

struct FrameInputs {
  wgpu::BindGroup frameBindGroup;
};

struct SceneInputs {
  wgpu::BindGroup sceneBindGroup;
};

struct Pipeline {
  wgpu::TextureFormat outputFormat;
  wgpu::RenderPipeline pipeline;

  FrameInputs create_frame_inputs(
      const wgpu::Device& device, const CameraCommonVsUbo& camera_common_vs_ubo,
      const CameraCommonFsUbo& camera_common_fs_ubo) const;
  SceneInputs create_scene_inputs(
      const wgpu::Device& device,
      const render::CommonLightingUbo& common_lighting_ubo) const;
};

class PipelineBuilder {
 public:
  static PipelineBuilder Create(const wgpu::Device& device,
                                const indigo::asset::pb::WgslSource& vs_src,
                                const indigo::asset::pb::WgslSource& fs_src);

  PipelineBuilder(wgpu::ShaderModule vert_module,
                  wgpu::ShaderModule frag_module, std::string vs_entry_point,
                  std::string fs_entry_point);

  Pipeline create_pipeline(const wgpu::Device& device,
                           wgpu::TextureFormat output_format) const;

 private:
  wgpu::ShaderModule vert_module_;
  wgpu::ShaderModule frag_module_;

  std::string vs_entry_point_;
  std::string fs_entry_point_;
};

// Utility used to actually submit draw calls, and make rendering a bit more
//  clean and readable. Should be used in a narrow scope and not outlive
//  RenderPassEncoder that is sent into the ctor.
class RenderUtil {
 public:
  RenderUtil(const wgpu::RenderPassEncoder* pass, const Pipeline* pipeline);

  RenderUtil& set_scene_inputs(const SceneInputs& inputs);
  RenderUtil& set_frame_inputs(const FrameInputs& inputs);
  RenderUtil& set_geometry(const Geo& geo);
  RenderUtil& set_instances(const wgpu::Buffer& buffer, uint32_t num_instances);
  RenderUtil& draw(bool* o_success = nullptr);

 private:
  const wgpu::RenderPassEncoder* pass_;
  const Pipeline* pipeline_;

  bool frame_inputs_set_;
  bool scene_inputs_set_;

  int32_t num_indices_;
  int32_t num_instances_;
};

}  // namespace sanctify::render::solid_static

#endif
