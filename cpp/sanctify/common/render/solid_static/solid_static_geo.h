#ifndef SANCTIFY_COMMON_RENDER_SOLID_STATIC_SOLID_STATIC_GEO_H
#define SANCTIFY_COMMON_RENDER_SOLID_STATIC_SOLID_STATIC_GEO_H

#include <common/render/common/camera_ubos.h>
#include <igasset/vertex_formats.h>
#include <igcore/pod_vector.h>
#include <webgpu/webgpu_cpp.h>

/**
 * Geometry type for stuff that can be rendered with the SolidStaticPipeline
 *
 * Consists of pretty standard vertex types:
 * - 3D Position
 * - TBN quaternion
 *
 * Standard index buffer as well
 *
 * Index buffer format contains both geometry and material data:
 * - World Transformation Matrix
 * - Albedo
 * - Normal
 * - Metallic
 * - Roughness
 * - Ambient Occlusion
 */

namespace sanctify::render::solid_static {

struct Geo {
  wgpu::Buffer geoVertexBuffer;
  wgpu::Buffer indexBuffer;
  int32_t numIndices;
  wgpu::IndexFormat indexFormat;

  Geo(const wgpu::Device& device,
      const indigo::core::PodVector<indigo::asset::PositionNormalVertexData>&
          vertices,
      const indigo::core::PodVector<uint32_t>& indices);
};

// Order to match the GPU resource (tightly pack floats at end of vec3s)
struct InstanceData {
  glm::mat4 matWorld;
  glm::vec3 albedo;
  float metallic;
  float roughness;
  float ambientOcclusion;
};

}  // namespace sanctify::render::solid_static

#endif
