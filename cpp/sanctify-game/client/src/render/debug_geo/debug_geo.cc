#include <igcore/math.h>
#include <iggpu/util.h>
#include <render/debug_geo/debug_geo.h>

using namespace sanctify;
using namespace debug_geo;

using namespace indigo;
using namespace core;
using namespace asset;

namespace {
uint32_t get_instance_capacity(uint32_t size) {
  if (size <= 1u) {
    return 1u;
  }

  if (size <= 16u) {
    return 16u;
  }

  if (size <= 64u) {
    return 64u;
  }

  return size;
}
}  // namespace

DebugGeo::DebugGeo(const wgpu::Device& device,
                   const PodVector<LegacyPositionNormalVertexData>& vertices,
                   const PodVector<uint32_t> indices)
    : GeoVertexBuffer(
          iggpu::buffer_from_data(device, vertices, wgpu::BufferUsage::Vertex)),
      IndexBuffer(
          iggpu::buffer_from_data(device, indices, wgpu::BufferUsage::Index)),
      NumIndices(indices.size()),
      IndexFormat(wgpu::IndexFormat::Uint32) {}

DebugGeo DebugGeo::CreateDebugUnitCube(const wgpu::Device& device) {
  core::PodVector<LegacyPositionNormalVertexData> vertices(24);
  core::PodVector<uint32_t> indices(36);

  // Back
  vertices.push_back({glm::vec3(-1.f, -1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)});
  vertices.push_back({glm::vec3(-1.f, 1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)});
  vertices.push_back({glm::vec3(1.f, -1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)});
  vertices.push_back({glm::vec3(1.f, 1.f, -1.f), glm::vec3(0.f, 0.f, -1.f)});
  indices.push_back(0, 1, 3, 0, 3, 2);

  // Front
  vertices.push_back({glm::vec3(-1.f, -1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)});
  vertices.push_back({glm::vec3(-1.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)});
  vertices.push_back({glm::vec3(1.f, -1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)});
  vertices.push_back({glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)});
  indices.push_back(4, 7, 5, 4, 6, 7);

  // Left
  vertices.push_back({glm::vec3(-1.f, -1.f, -1.f), glm::vec3(-1.f, 0.f, 0.f)});
  vertices.push_back({glm::vec3(-1.f, 1.f, -1.f), glm::vec3(-1.f, 0.f, 0.f)});
  vertices.push_back({glm::vec3(-1.f, 1.f, 1.f), glm::vec3(-1.f, 0.f, 0.f)});
  vertices.push_back({glm::vec3(-1.f, -1.f, 1.f), glm::vec3(-1.f, 0.f, 0.f)});
  indices.push_back(8, 9, 10, 8, 10, 11);

  // Right
  vertices.push_back({glm::vec3(1.f, -1.f, -1.f), glm::vec3(1.f, 0.f, 0.f)});
  vertices.push_back({glm::vec3(1.f, 1.f, -1.f), glm::vec3(1.f, 0.f, 0.f)});
  vertices.push_back({glm::vec3(1.f, 1.f, 1.f), glm::vec3(1.f, 0.f, 0.f)});
  vertices.push_back({glm::vec3(1.f, -1.f, 1.f), glm::vec3(1.f, 0.f, 0.f)});
  indices.push_back(12, 13, 14, 12, 14, 15);

  // Top
  vertices.push_back({glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 1.f, 0.f)});
  vertices.push_back({glm::vec3(-1.f, 1.f, 1.f), glm::vec3(0.f, 1.f, 0.f)});
  vertices.push_back({glm::vec3(-1.f, 1.f, -1.f), glm::vec3(0.f, 1.f, 0.f)});
  vertices.push_back({glm::vec3(1.f, 1.f, -1.f), glm::vec3(0.f, 1.f, 0.f)});
  indices.push_back(16, 18, 17, 16, 19, 18);

  // Bottom
  vertices.push_back({glm::vec3(1.f, -1.f, 1.f), glm::vec3(0.f, -1.f, 0.f)});
  vertices.push_back({glm::vec3(-1.f, -1.f, 1.f), glm::vec3(0.f, -1.f, 0.f)});
  vertices.push_back({glm::vec3(-1.f, -1.f, -1.f), glm::vec3(0.f, -1.f, 0.f)});
  vertices.push_back({glm::vec3(1.f, -1.f, -1.f), glm::vec3(0.f, -1.f, 0.f)});
  indices.push_back(20, 22, 21, 20, 23, 22);

  return DebugGeo(device, vertices, indices);
}

void InstanceBuffer::update_index_data(
    const wgpu::Device& device, const PodVector<InstanceData>& instance_data) {
  if (instance_data.size() == 0) {
    numInstances = 0;
  }

  if (instance_data.size() > capacity) {
    uint32_t new_capacity = ::get_instance_capacity(instance_data.size());
    instanceBuffer = iggpu::buffer_from_data(
        device, instance_data, wgpu::BufferUsage::Vertex,
        new_capacity * sizeof(InstanceData));
    capacity = new_capacity;
    numInstances = instance_data.size();
    return;
  }

  device.GetQueue().WriteBuffer(instanceBuffer, 0, &instance_data[0],
                                instance_data.raw_size());
  numInstances = instance_data.size();
}

InstanceBuffer::InstanceBuffer(const wgpu::Device& device)
    : instanceBuffer(iggpu::create_empty_buffer(
          device, sizeof(InstanceData) * 4, wgpu::BufferUsage::Vertex)),
      numInstances(0),
      capacity(4) {}

std::string InstanceKey::get_key() const {
  return std::to_string(geoKey.get_raw_key()) + ":" + std::to_string(lifetime);
}

int InstanceKey::get_lifetime() const { return lifetime; }
