#ifndef SANCTIFY_GAME_CLIENT_SRC_RENDER_DEBUG_GEO_DEBUG_GEO_H
#define SANCTIFY_GAME_CLIENT_SRC_RENDER_DEBUG_GEO_DEBUG_GEO_H

#include <igasset/vertex_formats.h>
#include <igcore/pod_vector.h>
#include <webgpu/webgpu_cpp.h>

namespace sanctify::debug_geo {

struct DebugGeo {
  wgpu::Buffer GeoVertexBuffer;
  wgpu::Buffer IndexBuffer;
  int32_t NumIndices;
  wgpu::IndexFormat IndexFormat;

  DebugGeo(
      const wgpu::Device& device,
      const indigo::core::PodVector<indigo::asset::LegacyPositionNormalVertexData>&
          vertices,
      const indigo::core::PodVector<uint32_t> indices);

  static DebugGeo CreateDebugUnitCube(const wgpu::Device& device);
};

struct InstanceData {
  glm::mat4 MatWorld;
  glm::vec3 ObjectColor;
};

struct InstanceBuffer {
  wgpu::Buffer instanceBuffer;
  int32_t numInstances;
  int32_t capacity;

  void update_index_data(
      const wgpu::Device& device,
      const indigo::core::PodVector<InstanceData>& instance_data);

  InstanceBuffer(const wgpu::Device& device);
};

}  // namespace sanctify::debug_geo

#endif
