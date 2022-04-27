#include "solid_static_geo.h"

#include <iggpu/util.h>

using namespace sanctify;
using namespace render;
using namespace solid_static;

using namespace indigo;
using namespace core;
using namespace asset;

Geo::Geo(const wgpu::Device& device,
         const PodVector<PositionNormalVertexData>& vertices,
         const PodVector<uint32_t>& indices)
    : geoVertexBuffer(
          iggpu::buffer_from_data(device, vertices, wgpu::BufferUsage::Vertex)),
      indexBuffer(
          iggpu::buffer_from_data(device, indices, wgpu::BufferUsage::Index)),
      numIndices(indices.size()),
      indexFormat(wgpu::IndexFormat::Uint32) {}
