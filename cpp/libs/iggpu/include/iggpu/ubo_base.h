#ifndef LIB_IGGPU_UBO_BASE_H
#define LIB_IGGPU_UBO_BASE_H

#include <igcore/dirtyable.h>
#include <iggpu/util.h>
#include <webgpu/webgpu_cpp.h>

namespace indigo::iggpu {

template <typename T>
class UboBase {
 public:
  UboBase() : buffer_(nullptr) {}
  UboBase(const wgpu::Device& device)
      : buffer_(create_empty_buffer(device, sizeof(T),
                                    wgpu::BufferUsage::Uniform)) {}
  UboBase(const wgpu::Device& device, const T& t)
      : buffer_(buffer_from_data(device, t, wgpu::BufferUsage::Uniform)) {}
  UboBase(const UboBase&) = delete;
  UboBase& operator=(const UboBase&) = delete;
  UboBase(UboBase&&) = default;
  UboBase& operator=(UboBase&&) = default;
  virtual ~UboBase() = default;

  T& get_mutable() { return data_; }
  const T& get_immutable() const { return data_; }

  uint32_t size() const { return sizeof(T); }

  void sync(const wgpu::Device& device) {
    if (data_.is_dirty()) {
      device.GetQueue().WriteBuffer(buffer(), 0, &data_.get(), sizeof(T));
      data_.clean();
    }
  }

  const wgpu::Buffer& buffer() const {
    assert(buffer_ != nullptr);
    return buffer_;
  }

 protected:
  wgpu::Buffer buffer_;
  core::DirtyablePod<T> data_;
};

}  // namespace indigo::iggpu

#endif
