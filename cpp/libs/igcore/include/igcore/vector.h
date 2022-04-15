#ifndef _LIB_IGCORE_VECTOR_H_
#define _LIB_IGCORE_VECTOR_H_

#include <igcore/ivec.h>
#include <igcore/log.h>

#include <cassert>
#include <cstring>

namespace indigo::core {

/**
 * Indigo Vector impelmentation - behaves fairly similarly to std::vector
 *
 * See further implementation notes in ivec.h
 */
template <typename T>
class Vector : public IVec<T> {
 public:
  Vector<T>(size_t size_hint = 2u)
      : data_(new T[size_hint]), IVec<T>(0u, size_hint) {}

  Vector<T>(const Vector<T>& o) : data_(new T[o.size()]), IVec<T>(0, o.size()) {
    for (size_t i = 0; i < o.size(); i++) {
      this->push_back(o[i]);
    }
  }

  Vector(Vector<T>&& o) noexcept
      : data_(o.data_), IVec<T>(o.size_, o.capacity_) {
    o.data_ = nullptr;
    o.size_ = 0;
    o.capacity_ = 0;
  }

  ~Vector() {
    if (data_ != nullptr) {
      delete[] data_;
    }
  }

  Vector<T>& operator=(Vector<T>&& o) {
    this->data_ = o.data_;
    this->size_ = o.size_;
    this->capacity_ = o.capacity_;

    o.data_ = nullptr;
    o.size_ = 0;
    o.capacity_ = 0;

    return *this;
  }

  T& operator[](size_t i) override { return data_[i]; }
  const T& operator[](size_t i) const override {
    assert(i < this->size_);
    return data_[i];
  }

  T& last() { return data_[this->size_ - 1]; }

  bool delete_at(size_t i, bool preserve_order = true) override {
    if (i >= this->size_) return false;

    if (preserve_order) {
      for (size_t j = i + 1; j < this->size_; j++) {
        this->data_[j - 1] = std::move(this->data_[j]);
      }
      this->data_[this->size_-- - 1] = T{};
    } else {
      this->data_[i] = std::move(this->data_[this->size_ - 1]);
      this->size_--;
      this->data_[this->size_] = T{};
    }

    return true;
  }

  /** Trim the first n elements from this vector */
  void trim_front(uint32_t i) {
    if (this->size_ <= i) {
      this->size_ = 0;
      return;
    }

    auto* old_data = this->data_;
    this->size_ -= i;
    this->capacity_ -= i;
    this->data_ = new T[this->capacity_];
    for (int i = 0; i < this->size_; i++) {
      this->data_[i] = std::move(old_data[i]);
    }
    delete[] old_data;
  }

  void clear() {
    for (size_t i = 0; i < this->size_; i++) {
      this->data_[i] = T{};
    }
    this->size_ = 0;
  }

  /**
   * Return a new Vector instance containing the objects that match the given
   * predicate function, transformed using the given transformation function.
   * New vector is populated via copy - type T must be copyable.
   */
  template <typename ReturnType, typename FilterFn, typename MapFn>
  Vector<ReturnType> map_filter(FilterFn predicate, MapFn transform) const {
    Vector<ReturnType> r;
    for (size_t i = 0; i < this->size_; i++) {
      auto& ref = operator[](i);
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
  Vector<T> filter(FilterFn predicate) const {
    Vector<T> r;
    for (size_t i = 0; i < this->size_; i++) {
      auto& ref = operator[](i);
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
  Vector<ReturnType> map(MapFn transform) const {
    Vector<ReturnType> r;
    for (size_t i = 0; i < this->size_; i++) {
      const T& ref = operator[](i);
      r.push_back(transform(ref));
    }
    return std::move(r);
  }

 protected:
  void alloc_to(size_t size) override {
    if (this->size_ < this->capacity_) {
      return;
    }

    if (this->capacity_ == 0) {
      this->data_ = new T[size];
      this->capacity_ = size;
    }

    size_t old_size = this->capacity_;
    while (this->size_ >= this->capacity_) {
      if (this->capacity_ < 256) {
        this->capacity_ *= 2;
      } else if (this->capacity_ < 512) {
        this->capacity_ = 512;
      } else {
        this->capacity_ += 256;
      }
    }
    T* old = this->data_;
    this->data_ = new T[this->capacity_];
    for (size_t i = 0; i < this->size_; i++) {
      this->data_[i] = std::move(old[i]);
    }
    delete[] old;
  }

  void swap(T* a, T* b) override {
    T tmp = std::move(*a);
    *a = std::move(*b);
    *b = std::move(tmp);
  }

 private:
  T* data_;
};

}  // namespace indigo::core

#endif
