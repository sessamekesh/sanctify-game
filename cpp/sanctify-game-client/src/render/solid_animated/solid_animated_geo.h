#ifndef SANCTIFY_GAME_CLIENT_SRC_RENDER_SOLID_ANIMATED_SOLID_ANIMATED_GEO_H
#define SANCTIFY_GAME_CLIENT_SRC_RENDER_SOLID_ANIMATED_SOLID_ANIMATED_GEO_H

#include <igasset/vertex_formats.h>
#include <igcore/pod_vector.h>
#include <webgpu/webgpu_cpp.h>

namespace sanctify::solid_animated {

struct SolidAnimatedGeo {
  wgpu::Buffer GeoVertexBuffer;
  wgpu::Buffer IndexBuffer;
  int32_t NumIndices;
  wgpu::IndexFormat IndexFormat;

  SolidAnimatedGeo(
      const wgpu::Device& device,
      const indigo::core::PodVector<indigo::asset::PositionNormalVertexData>&
          vertices,
      const indigo::core::PodVector<uint32_t> indices);
};

struct MatWorldInstanceData {
  glm::mat4 MatWorld;
};

struct MatWorldInstanceBuffer {
  wgpu::Buffer InstanceBuffer;
  int32_t NumInstances;
  int32_t Capacity;

  void update_index_data(
      const wgpu::Device& device,
      const indigo::core::PodVector<MatWorldInstanceData>& instance_data);

  MatWorldInstanceBuffer(
      const wgpu::Device& device,
      const indigo::core::PodVector<MatWorldInstanceData>& instance_data);
};

}  // namespace sanctify::solid_animated

#endif
