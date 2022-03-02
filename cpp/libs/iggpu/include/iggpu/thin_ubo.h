#include <igcore/log.h>
#include <igcore/pod_vector.h>
#include <iggpu/util.h>
#include <webgpu/webgpu_cpp.h>

namespace indigo::iggpu {

/**
 * Thin UBO implementation - knows its size and keeps a reference to a GPU
 * buffer, that's it.
 */

class ThinUbo {
 public:
  ThinUbo(uint32_t size, const wgpu::Device& device)
      : buffer_(create_empty_buffer(device, size, wgpu::BufferUsage::Uniform)),
        size_(size) {}

  template <typename T>
  ThinUbo(const wgpu::Device& device, const T& data)
      : buffer_(buffer_from_data(device, data, wgpu::BufferUsage::Uniform)),
        size_(sizeof(data)) {}

  template <typename T>
  ThinUbo(const wgpu::Device& device, const core::PodVector<T>& data)
      : buffer_(buffer_from_data(device, data, wgpu::BufferUsage::Uniform)),
        size_(data.raw_size()) {}

  uint32_t size() const { return size_; }
  const wgpu::Buffer& buffer() const { return buffer_; }

  template <typename T>
  void update(const wgpu::Device& device, const core::PodVector<T>& data) {
    if (data.size() == 0) return;

    if (data.raw_size() > size_) {
      indigo::core::Logger::err("ThinUbo")
          << "Cannot update ThinUbo with data exceeding original size";
      return;
    }

    device.GetQueue().WriteBuffer(buffer_, 0, &data[0], data.raw_size());
  }

 private:
  wgpu::Buffer buffer_;
  uint32_t size_;
};

}  // namespace indigo::iggpu