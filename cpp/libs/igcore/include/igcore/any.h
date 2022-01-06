#ifndef _LIB_IGCORE_ANY_H_
#define _LIB_IGCORE_ANY_H_

#include <igcore/typeid.h>

#include <any>

namespace indigo::core {

/**
 * Exception-safe wrapper around std::any
 *
 * std::any does not provide any sort of try_cast - this type does
 * (hacky) reflection to keep track of what type is being stored
 */

class any {
 public:
  template <typename T>
  any(T t) : inner_(std::move(t)), type_id_(TypeId::of<T>()) {}

  template <typename T>
  bool try_get(T& o) const {
    if (type_id_ != TypeId::of<T>()) {
      return false;
    }
    o = std::any_cast<T>(inner_);
    return true;
  }

  template <typename T>
  bool is_type() const {
    return type_id_ == TypeId::of<T>();
  }

  template <typename T>
  const T* get() const {
    return std::any_cast<T>(&inner_);
  }

  uint32_t get_type_id() const { return type_id_; }

 private:
  std::any inner_;
  uint32_t type_id_;
};

}  // namespace indigo::core

#endif
