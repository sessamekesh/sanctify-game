#ifndef TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_BUILDER_H
#define TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_BUILDER_H

#include <igcore/maybe.h>
#include <util/assimp_loader.h>
#include <util/recast_params.h>
#include <webgpu/webgpu_cpp.h>

namespace mapeditor {

struct Renderable {
  std::string assimpFileName;
  std::string meshName;

  wgpu::Buffer PositionVertexBuffer;
  wgpu::Buffer NormalVertexBuffer;
  wgpu::Buffer IndexBuffer;
  uint32_t NumIndices;

  glm::mat4 worldTransform;
  glm::vec4 materialColor;

  int modelOffset;

  wgpu::BindGroup bindGroup;
};

struct LightingParams {
  glm::vec3 lightDirection;
  float ambientCoefficient;
  glm::vec3 lightColor;
  float specularPower;
};

struct VsCameraParams {
  glm::mat4 matView;
  glm::mat4 matProj;
};

struct FsCameraParams {
  glm::vec3 position;
};

class RecastBuilder {
 public:
  RecastBuilder(wgpu::Device device);

  void rebuild_from(std::shared_ptr<RecastParams> params,
                    std::shared_ptr<AssimpLoader> loader);

  bool is_valid_state() const;

  void set_lighting_params(LightingParams lighting_params);

  void render(glm::mat4 matView, glm::mat4 matProj, glm::vec3 cameraPos,
              wgpu::TextureView target, wgpu::TextureView depthBuffer);

 private:
  indigo::core::Maybe<Renderable> build_from_op(
      const indigo::igpackgen::pb::AssembleRecastNavMeshAction_AssimpGeoDef&
          def,
      glm::vec4 color, std::shared_ptr<RecastParams> params,
      std::shared_ptr<AssimpLoader> loader);

 private:
  wgpu::Device device_;

  std::vector<Renderable> renderables_;

  wgpu::RenderPipeline map_preview_pipeline_;

  wgpu::Buffer lighting_params_buffer_;
  wgpu::Buffer camera_vert_params_buffer_;
  wgpu::Buffer camera_frag_params_buffer_;
  wgpu::Buffer model_transform_buffer_;
  wgpu::Buffer model_color_buffer_;
  int model_buffer_capacity_;
  int next_model_offset_;
};

}  // namespace mapeditor

#endif
