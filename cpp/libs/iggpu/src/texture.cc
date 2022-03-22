#include <iggpu/texture.h>
#include <iggpu/util.h>

using namespace indigo;
using namespace iggpu;

wgpu::TextureView Texture::create_default_view() const {
  wgpu::TextureViewDescriptor desc = iggpu::view_desc_of(*this);
  return GpuTexture.CreateView(&desc);
}
