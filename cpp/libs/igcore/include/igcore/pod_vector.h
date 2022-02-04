#ifndef _LIB_IGCORE_POD_VECTOR_H_
#define _LIB_IGCORE_POD_VECTOR_H_

#include <igcore/config.h>
#include <igcore/ivec.h>
#include <igcore/raw_buffer.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace indigo::core {

// DO NOT USE FOR ANYTHING BUT POD TYPES!!!!!!
// Number of times using PodVector for non-POD types has caused grief: 3

/**
 * Plain Old Data (POD) Vector type
 *
 * This is ONLY safe to use with trivial POD types! If there is any tricky
 * construction, destruction, copy, or move semantics with the underlying type,
 * you WILL see bizarre bugs!
 *
 * -------------------------- IMPLEMENTATION NOTES ---------------------------
 * - A single instance of T must fit on the heap
 * - Instances of T must be trivially created, copied, moved, and destroyed
 */

template <typename T>
class PodVector : public IVec<T> {
 public:
  // Notice: Allocations are done using raw byte type instead of T*, this is
  // to avoid calling constructors/destructors on the underlying data. Callers
  // must make sure to instantiate each individual element as they are passed
  // in, by push_back or by assignment operator against the reference returned
  // to operator[]
  PodVector<T>(size_t size_hint = 2u)
      : data_(new uint8_t[(size_hint + 1) * sizeof(T)]),
        IVec<T>(0u, size_hint + 1) {
    static_assert(std::is_pod<T>::value,
                  "PodVector<T> must only be used with Plain-Old-Data types!");
#ifdef IG_ZERO_NEW_ALLOCATIONS
    memset(data_, 0x00, this->raw_size());
#endif
  }

  ~PodVector() {
    if (data_ != nullptr) {
      delete[] data_;
      data_ = nullptr;
    }
  }

  PodVector<T>(const PodVector<T> &o)
      : data_(new uint8_t[o.size_ * sizeof(T)]), IVec<T>(o.size_, o.capacity_) {
    memcpy(data_, o.data_, o.size_ * sizeof(T));
  }

  PodVector<T> &operator=(const PodVector<T> &o) {
    delete[] data_;
    this->data_ = new uint8_t[o.size_ * sizeof(T)];
    this->size_ = o.size_;
    this->capacity_ = o.capacity_;
    memcpy(this->data_, o.data_, o.size_ * sizeof(T));
    return *this;
  }

  PodVector<T>(PodVector<T> &&o) noexcept
      : data_(o.data_), IVec<T>(o.size_, o.capacity_) {
    o.data_ = nullptr;
    o.size_ = 0;
    o.capacity_ = 0;
  }

  PodVector<T> &operator=(PodVector<T> &&o) {
    this->data_ = o.data_;
    this->size_ = o.size_;
    this->capacity_ = o.capacity_;
    o.data_ = nullptr;
    o.size_ = 0;
    o.capacity_ = 0;
    return *this;
  }

  static PodVector<T> from(const IVec<T> &o) {
    PodVector<T> tr(o.size());
    for (size_t i = 0; i < o.size(); i++) {
      tr.push_back(o[i]);
    }
    return std::move(tr);
  }

  T &operator[](size_t i) override {
    assert(i < this->size_);
    return typed_data_[i];
  }
  const T &operator[](size_t i) const override {
    assert(i < this->size_);
    return typed_data_[i];
  }

  bool delete_at(size_t i, bool preserve_order = true) override {
    if (i >= this->size_) return false;  // Do not delete OOB

    // NOTICE!!!! NO destructors are called here! Data MUST BE PODS!!!
    // If type T is NOT a plain-old-data type, use core::Vector instead
    if (preserve_order) {
      memcpy(this->data_ + sizeof(T) * i, this->data_ + sizeof(T) * (i + 1),
             (this->size_ - i) * sizeof(T));
    } else {
      // TODO (sessamekesh): Investigate a memory corruption problem in this
      //  segment (possibly in above segment too)
      memcpy(this->data_ + sizeof(T) * i,
             this->data_ + sizeof(T) * (this->size_ - 1), sizeof(T));
    }

#ifdef IG_ZERO_NEW_ALLOCATIONS
    memset(this->data_ + sizeof(T) * this->size_, 0x00, sizeof(T));
#endif

    this->size_--;
    return true;
  }

  /**
   * Return a new Vector instance containing the objects that match the given
   * predicate function, transformed using the given transformation function.
   * New vector is populated via copy - type T must be copyable.
   */
  template <typename ReturnType, typename FilterFn, typename MapFn>
  PodVector<ReturnType> map_filter(FilterFn predicate, MapFn transform) const {
    PodVector<ReturnType> r;
    for (size_t i = 0; i < this->size_; i++) {
      auto &ref = operator[](i);
      if (predicate(ref)) {
        r.push_back(transform(ref));
      }
    }
    return std::move(r);
  }

  /**
   * Return a new Vector instance containing the objects that match the given
   * predicate function. New vector is populated via copy - type T must be
   * copyable.
   */
  template <typename FilterFn>
  PodVector<T> filter(FilterFn predicate) const {
    PodVector<T> r;
    for (size_t i = 0; i < this->size_; i++) {
      auto &ref = operator[](i);
      if (predicate(ref)) {
        r.push_back(ref);
      }
    }
    return std::move(r);
  }

  /**
   * Return a new Vector instance containing a list of elements which correspond
   * to the list of elements in this vector, after a transformation function is
   * applied to it.
   */
  template <typename ReturnType, typename MapFn>
  PodVector<ReturnType> map(MapFn transform) const {
    PodVector<ReturnType> r;
    for (size_t i = 0; i < this->size_; i++) {
      const T &ref = operator[](i);
      r.push_back(transform(ref));
    }
    return std::move(r);
  }

  /**
   * Extract the raw buffer from this PodVector - this destroys the PodVector
   * in the process, transferring memory to the new raw buffer
   */
  core::RawBuffer move_to_raw() {
    core::RawBuffer b(data_, this->raw_size());
    this->data_ = nullptr;
    this->size_ = 0u;
    this->capacity_ = 0u;
    return std::move(b);
  }

  /**
   * Resize this vector - basically allow operator[] at any point inside the
   * buffer, because this is a POD vector there isn't any construction that
   * happens here.
   */
  void resize(size_t new_size) {
    alloc_to(new_size);
#ifdef IG_ZERO_NEW_ALLOCATIONS
    memset(this->data_ + this->size_ * sizeof(T), 0x00,
           (this->capacity_ - this->size_) * sizeof(T));
#endif
    this->size_ = new_size;
  }

 protected:
  void alloc_to(size_t size) override {
    if (size < this->capacity_) {
      return;
    }

    uint32_t old_size = (uint32_t)this->capacity_;
    while (size >= this->capacity_) {
      if (this->capacity_ < 256) {
        this->capacity_ *= 2;
      } else if (this->capacity_ < 512) {
        this->capacity_ = 512;
      } else {
        this->capacity_ += 256;
      }
    }

    uint8_t *old = this->data_;
    this->data_ = new uint8_t[this->capacity_ * sizeof(T)];
    if (old) {
      memcpy(reinterpret_cast<void *>(this->data_),
             reinterpret_cast<void *>(old), old_size * sizeof(T));
#ifdef IG_ZERO_NEW_ALLOCATIONS
      memset(this->data_ + old_size * sizeof(T), 0x00,
             (this->capacity_ - old_size) * sizeof(T));
#endif
      delete[] old;
    }
  }

  void swap(T *a, T *b) override {
    uint8_t t[sizeof(T)]{};
    memcpy(t, a, sizeof(T));
    memcpy(a, b, sizeof(T));
    memcpy(b, t, sizeof(T));
  }

 private:
  union {
    uint8_t *data_;
    T *typed_data_;
  };
};

}  // namespace indigo::core

#endif
