#ifndef LIB_IGGPU_COMMON_TEXTURE_H
#define LIB_IGGPU_COMMON_TEXTURE_H

#include <webgpu/webgpu_cpp.h>

namespace indigo::iggpu {

struct Texture {
  uint32_t Width;
  uint32_t Height;
  uint32_t MipLevels;
  wgpu::TextureFormat Format;
  wgpu::Texture GpuTexture;

  wgpu::TextureView create_default_view() const;
};

struct Texture3D {
  uint32_t Width;
  uint32_t Height;
  uint32_t Depth;
  uint32_t MipLevels;

  wgpu::TextureFormat Format;
  wgpu::Texture GpuTexture;
};

struct TextureCube {
  uint32_t Width;
  uint32_t Height;
  uint32_t MipLevels;

  wgpu::TextureFormat Format;
  wgpu::Texture GpuTexture;
};

}  // namespace indigo::iggpu

#endif
