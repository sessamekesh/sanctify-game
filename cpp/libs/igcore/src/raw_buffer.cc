#include <igcore/config.h>
#include <igcore/log.h>
#include <igcore/raw_buffer.h>

#include <cstring>

namespace {
const char* kLogLabel = "RawBuffer";
}

namespace indigo {
namespace core {

RawBuffer::RawBuffer(size_t size)
    : data_(size == 0 ? nullptr : new uint8_t[size]),
      size_(size),
      owns_data_(true) {
#ifdef IG_ZERO_NEW_ALLOCATIONS
  memset(data_, 0x00, size_);
#endif
}

RawBuffer::RawBuffer(uint8_t* data, size_t size, bool owns_data)
    : data_(data), size_(size), owns_data_(owns_data) {}

RawBuffer::~RawBuffer() {
  if (data_ && owns_data_) {
    delete[] data_;
    size_ = 0;
  }
}

RawBuffer::RawBuffer(const RawBuffer& o) {
  if (o.size_ > 1024) {
    core::Logger::log(kLogLabel) << "WARNING - copy of size " << o.size_
                                 << " taking place implicitly. Please use "
                                    ".clone() if this is intentional";
  }
  data_ = new uint8_t[o.size_];
  size_ = o.size_;
  owns_data_ = o.owns_data_;
  memcpy(data_, o.data_, o.size_);
}

RawBuffer RawBuffer::clone() const {
  RawBuffer o(size_);
  memcpy(o.data_, data_, size_);
  return std::move(o);
}

RawBuffer::RawBuffer(RawBuffer&& o) noexcept {
  data_ = o.data_;
  size_ = o.size_;
  owns_data_ = o.owns_data_;
  o.data_ = nullptr;
  o.size_ = 0;
}

RawBuffer& RawBuffer::operator=(const RawBuffer& o) {
  if (data_) {
    delete[] data_;
    data_ = nullptr;
  }
  data_ = new uint8_t[o.size_];
  size_ = o.size_;
  owns_data_ = o.owns_data_;
  memcpy(data_, o.data_, o.size_);
  return *this;
}

RawBuffer& RawBuffer::operator=(RawBuffer&& o) noexcept {
  if (data_) {
    delete[] data_;
    data_ = nullptr;
  }
  data_ = o.data_;
  size_ = o.size_;
  owns_data_ = o.owns_data_;
  o.data_ = nullptr;
  o.size_ = 0;
  return *this;
}

uint8_t* RawBuffer::get() const { return data_; }

const size_t RawBuffer::size() const { return size_; }

uint8_t& RawBuffer::operator[](size_t size) const { return data_[size]; }

}  // namespace core
}  // namespace indigo