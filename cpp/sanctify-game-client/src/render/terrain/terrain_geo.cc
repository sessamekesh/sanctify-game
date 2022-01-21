#include <iggpu/util.h>
#include <render/terrain/terrain_geo.h>

using namespace sanctify;
using namespace terrain_pipeline;

using namespace indigo;
using namespace core;
using namespace asset;

namespace {
uint32_t get_instance_capacity(uint32_t size) {
  if (size <= 1u) {
    return 1u;
  }

  if (size <= 8u) {
    return 8u;
  }

  if (size <= 32u) {
    return 32u;
  }

  if (size <= 64u) {
    return 64u;
  }

  return size;
}
}  // namespace

TerrainGeo::TerrainGeo(const wgpu::Device& device,
                       const PodVector<PositionNormalVertexData>& vertices,
                       const PodVector<uint32_t>& indices)
    : GeoVertexBuffer(
          iggpu::buffer_from_data(device, vertices, wgpu::BufferUsage::Vertex)),
      IndexBuffer(
          iggpu::buffer_from_data(device, indices, wgpu::BufferUsage::Index)),
      NumIndices(indices.size()),
      IndexFormat(wgpu::IndexFormat::Uint32) {}

void TerrainMatWorldInstanceBuffer::update_index_data(
    const wgpu::Device& device,
    const PodVector<TerrainMatWorldInstanceData>& instance_data) {
  if (instance_data.size() > Capacity) {
    uint32_t new_capacity = ::get_instance_capacity(instance_data.size());
    InstanceBuffer = iggpu::buffer_from_data(
        device, instance_data, wgpu::BufferUsage::Vertex,
        new_capacity * sizeof(TerrainMatWorldInstanceData));
    Capacity = new_capacity;
    NumInstances = instance_data.size();
    return;
  }

  device.GetQueue().WriteBuffer(InstanceBuffer, 0, &instance_data[0],
                                instance_data.raw_size());
  NumInstances = instance_data.size();
  return;
}

TerrainMatWorldInstanceBuffer::TerrainMatWorldInstanceBuffer(
    const wgpu::Device& device,
    const PodVector<TerrainMatWorldInstanceData>& data)
    : InstanceBuffer(
          iggpu::buffer_from_data(device, data, wgpu::BufferUsage::Vertex)),
      NumInstances(data.size()),
      Capacity(data.size()) {}