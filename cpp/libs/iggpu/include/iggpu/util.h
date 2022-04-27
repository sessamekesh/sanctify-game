#ifndef LIB_IGGPU_UTIL_H
#define LIB_IGGPU_UTIL_H

#include <igasync/promise.h>
#include <igcore/either.h>
#include <igcore/pod_vector.h>
#include <igcore/vector.h>
#include <iggpu/texture.h>
#include <webgpu/webgpu_cpp.h>

#include <string>

namespace indigo::iggpu {

enum class BuildShaderError {
  CompileError,
  DeviceLost,
};

wgpu::Buffer create_empty_buffer(const wgpu::Device& device, uint32_t size,
                                 wgpu::BufferUsage usage);

template <typename T>
wgpu::Buffer buffer_from_data(const wgpu::Device& device, const T& data,
                              wgpu::BufferUsage usage) {
  wgpu::BufferDescriptor desc{};
  desc.size = sizeof(data);
  desc.usage = usage | wgpu::BufferUsage::CopyDst;
  desc.mappedAtCreation = true;

  wgpu::Buffer buffer = device.CreateBuffer(&desc);

  void* ptr = buffer.GetMappedRange();
  memcpy(ptr, &data, sizeof(data));
  buffer.Unmap();

  return buffer;
}

template <typename T>
wgpu::Buffer buffer_from_data(const wgpu::Device& device,
                              const core::PodVector<T>& data,
                              wgpu::BufferUsage usage,
                              uint32_t size_override = 0) {
  wgpu::BufferDescriptor desc{};
  if (size_override == 0) {
    desc.size = data.raw_size();
  } else {
    desc.size = size_override;
  }
  desc.usage = usage | wgpu::BufferUsage::CopyDst;
  desc.mappedAtCreation = true;

  wgpu::Buffer buffer = device.CreateBuffer(&desc);

  void* ptr = buffer.GetMappedRange();
  memcpy(ptr, &data[0], data.raw_size());
  buffer.Unmap();

  return buffer;
}

wgpu::ShaderModule create_shader_module(const wgpu::Device& device,
                                        const std::string& wgsl_src,
                                        const char* debug_name = "");

// Bind group creation
wgpu::BindGroupDescriptor bind_group_desc(
    const core::Vector<wgpu::BindGroupEntry>& entries,
    const wgpu::BindGroupLayout& layout, const char* debug_name = "");

wgpu::BindGroupEntry buffer_bind_group_entry(uint32_t binding,
                                             const wgpu::Buffer& buffer,
                                             uint32_t size,
                                             uint32_t offset = 0u);

Texture create_empty_texture_2d(const wgpu::Device& device, uint32_t width,
                                uint32_t height, uint32_t mip_levels,
                                wgpu::TextureFormat format,
                                wgpu::TextureUsage usage);

wgpu::TextureViewDescriptor view_desc_of(const iggpu::Texture& texture);

wgpu::VertexAttribute vertex_attribute(uint32_t shader_location,
                                       wgpu::VertexFormat format,
                                       uint32_t offset);

wgpu::VertexBufferLayout vertex_buffer_layout(
    const core::PodVector<wgpu::VertexAttribute>& attributes, size_t size,
    wgpu::VertexStepMode vertex_step_mode);

wgpu::DepthStencilState depth_stencil_state_standard();

}  // namespace indigo::iggpu

#endif
