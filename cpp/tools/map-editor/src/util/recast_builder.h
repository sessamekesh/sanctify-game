#ifndef TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_BUILDER_H
#define TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_BUILDER_H

#include <igcore/maybe.h>
#include <ignav/recast_compiler.h>
#include <util/assimp_loader.h>
#include <util/recast_params.h>
#include <webgpu/webgpu_cpp.h>

namespace mapeditor {

struct Renderable {
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
              wgpu::TextureView target, wgpu::TextureView depthBuffer,
              bool render_assimp_geo, bool render_navmesh_geo);

 private:
  struct GeoExtraction {
    indigo::core::PodVector<glm::vec3> positions;
    indigo::core::PodVector<glm::vec3> normals;
    indigo::core::PodVector<uint32_t> indices;
  };

  indigo::core::Maybe<GeoExtraction> extract_geo(
      const indigo::igpackgen::pb::AssembleRecastNavMeshAction_AssimpGeoDef&
          def,
      std::shared_ptr<RecastParams> params,
      std::shared_ptr<AssimpLoader> loader);

  bool add_include_geo_to_compiler(indigo::nav::RecastCompiler& compiler,
                                   const GeoExtraction& geo);

  indigo::core::Maybe<Renderable> build_from_geo(glm::vec4 color,
                                                 const GeoExtraction& geo);
  void add_navmesh_renderable(const indigo::nav::DetourNavmesh& detour_navmesh);

 private:
  wgpu::Device device_;

  std::vector<Renderable> assimp_renderables_;
  std::vector<Renderable> recast_renderables_;

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
