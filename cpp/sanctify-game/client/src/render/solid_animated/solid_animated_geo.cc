#include <iggpu/util.h>
#include <render/solid_animated/solid_animated_geo.h>

using namespace indigo;
using namespace core;

using namespace sanctify;
using namespace solid_animated;

namespace {
uint32_t get_instance_capacity(uint32_t size) {
  if (size <= 1u) {
    return 1u;
  }

  if (size <= 32u) {
    return 32u;
  }

  if (size <= 256u) {
    return 256u;
  }

  return size;
}
}  // namespace

SolidAnimatedGeo::SolidAnimatedGeo(
    const wgpu::Device& device,
    const indigo::core::PodVector<indigo::asset::PositionNormalVertexData>&
        vertices,
    const indigo::core::PodVector<indigo::asset::SkeletalAnimationVertexData>&
        animation_data,
    const indigo::core::PodVector<uint32_t>& indices)
    : GeoVertexBuffer(
          iggpu::buffer_from_data(device, vertices, wgpu::BufferUsage::Vertex)),
      AnimationVertexBuffer(iggpu::buffer_from_data(device, animation_data,
                                                    wgpu::BufferUsage::Vertex)),
      IndexBuffer(
          iggpu::buffer_from_data(device, indices, wgpu::BufferUsage::Index)),
      NumIndices(indices.size()),
      IndexFormat(wgpu::IndexFormat::Uint32) {}

void MatWorldInstanceBuffer::update_index_data(
    const wgpu::Device& device,
    const PodVector<MatWorldInstanceData>& instance_data) {
  if (instance_data.size() == 0) {
    NumInstances = 0;
  }

  if (instance_data.size() > Capacity) {
    uint32_t new_capacity = ::get_instance_capacity(instance_data.size());
    InstanceBuffer = iggpu::buffer_from_data(
        device, instance_data, wgpu::BufferUsage::Vertex,
        new_capacity * sizeof(MatWorldInstanceData));
    Capacity = new_capacity;
    NumInstances = instance_data.size();
    return;
  }

  device.GetQueue().WriteBuffer(InstanceBuffer, 0, &instance_data[0],
                                instance_data.raw_size());
  NumInstances = instance_data.size();
}

MatWorldInstanceBuffer::MatWorldInstanceBuffer(
    const wgpu::Device& device, const PodVector<MatWorldInstanceData>& data)
    : InstanceBuffer(
          iggpu::buffer_from_data(device, data, wgpu::BufferUsage::Vertex)),
      NumInstances(data.size()),
      Capacity(data.size()) {}