#ifndef SANCTIFY_GAME_CLIENT_SRC_RENDER_TERRAIN_TERRAIN_GEO_H
#define SANCTIFY_GAME_CLIENT_SRC_RENDER_TERRAIN_TERRAIN_GEO_H

#include <igasset/vertex_formats.h>
#include <igcore/pod_vector.h>
#include <webgpu/webgpu_cpp.h>

namespace sanctify::terrain_pipeline {

struct TerrainGeo {
  wgpu::Buffer GeoVertexBuffer;
  wgpu::Buffer IndexBuffer;
  int32_t NumIndices;
  wgpu::IndexFormat IndexFormat;

  TerrainGeo(
      const wgpu::Device& device,
      const indigo::core::PodVector<indigo::asset::PositionNormalVertexData>&
          vertices,
      const indigo::core::PodVector<uint32_t>& indices);
};

struct TerrainMatWorldInstanceData {
  glm::mat4 MatWorld;
};

struct TerrainMatWorldInstanceBuffer {
  wgpu::Buffer InstanceBuffer;
  int32_t NumInstances;
  int32_t Capacity;

  void update_index_data(
      const wgpu::Device& device,
      const indigo::core::PodVector<TerrainMatWorldInstanceData>&
          instance_data);

  TerrainMatWorldInstanceBuffer(
      const wgpu::Device& device,
      const indigo::core::PodVector<TerrainMatWorldInstanceData>&
          instance_data);
};

}  // namespace sanctify::terrain_pipeline

#endif
