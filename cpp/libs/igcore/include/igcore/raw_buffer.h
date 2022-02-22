#ifndef _LIB_IGCORE_RAW_BUFFER_H_
#define _LIB_IGCORE_RAW_BUFFER_H_

#include <cstddef>
#include <cstdint>

namespace indigo::core {

// Wrapper around a raw data buffer (uint8_t*). Use this instead of
// std::vector<uint8_t> because of the mysterious super long setup/destroy time
// on web platform: https://github.com/sessamekesh/slow-wasm-vec-dtor

class RawBuffer {
 public:
  RawBuffer(size_t size);
  RawBuffer(uint8_t* data, size_t size, bool owns_data = true);
  ~RawBuffer();
  RawBuffer(const RawBuffer& o);
  RawBuffer(RawBuffer&& o) noexcept;
  RawBuffer& operator=(const RawBuffer& o);
  RawBuffer& operator=(RawBuffer&& o) noexcept;
  operator const uint8_t*() const { return data_; }
  uint8_t& operator[](size_t size) const;

  uint8_t* get() const;
  const size_t size() const;

  RawBuffer clone() const;

  bool detach(uint8_t** o_data, size_t* o_size);

 private:
  uint8_t* data_;
  size_t size_;
  bool owns_data_;
};

}  // namespace indigo::core

#endif
