#ifndef LIB_IGGPU_UTIL_H
#define LIB_IGGPU_UTIL_H

#include <igcore/vector.h>
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

}  // namespace indigo::iggpu

#endif
