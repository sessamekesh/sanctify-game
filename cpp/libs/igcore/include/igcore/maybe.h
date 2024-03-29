#ifndef _LIB_IGCORE_MAYBE_H_
#define _LIB_IGCORE_MAYBE_H_

#include <cstdint>
#include <functional>
#include <type_traits>

/**
 * Implementation of an optional type for Indigo uses
 *
 * A "Maybe" either exists or does not, and provides no bias towards whether or
 *  not it exists. It does provide a method to extract the value from the
 *  implementation, but this should only be done if a value is present!
 */

namespace indigo::core {

struct empty_maybe {};

template <typename T>
class Maybe {
 public:
  Maybe() : dummy_(0x00), is_initialized_(false) {}

  static Maybe<T> empty() { return Maybe<T>(); }

  Maybe(empty_maybe&&) : Maybe() {}

  Maybe(Maybe<T>&& o) noexcept : dummy_(0x00), is_initialized_(false) {
    static_assert(std::is_move_constructible<T>::value,
                  "Cannot create move constructor for a non-movable Maybe<T>");
    if (o.is_initialized_) {
      ::new (&value_) T(std::move(o.value_));
      is_initialized_ = true;
    }

    o.dummy_ = 0x00;
    o.is_initialized_ = false;
  }

  Maybe& operator=(Maybe<T>&& o) noexcept {
    static_assert(std::is_move_constructible<T>::value,
                  "Cannot create move constructor for a non-movable Maybe<T>");

    destroy_self();

    if (o.is_initialized_) {
      ::new (&value_) T(std::move(o.value_));
      is_initialized_ = true;
    } else {
      dummy_ = 0x00;
      is_initialized_ = false;
    }

    o.dummy_ = 0x00;
    o.is_initialized_ = false;

    return *this;
  }

  Maybe(T&& o) {
    static_assert(std::is_move_constructible<T>::value,
                  "Cannot create move constructor for a non-movable T");
    ::new (&value_) T(std::move(o));
    is_initialized_ = true;
  }

  Maybe& operator=(T&& o) {
    static_assert(
        std::is_move_assignable<T>::value,
        "Cannot create move assignment operator for a non-movable Maybe<T>");

    destroy_self();

    ::new (&value_) T(std::move(o));
    is_initialized_ = true;

    return *this;
  }

  Maybe(const Maybe<T>& o) : dummy_(0x00), is_initialized_(false) {
    static_assert(std::is_copy_constructible<T>::value,
                  "Cannot create copy constructor for a non-copyable Maybe<T>");
    if (o.is_initialized_) {
      ::new (&value_) T(o.value_);
      is_initialized_ = true;
    }
  }

  Maybe& operator=(const Maybe<T>& o) {
    static_assert(
        std::is_copy_constructible<T>::value,
        "Cannot create copy assignment operator for non-copyable Maybe<T>");

    destroy_self();

    if (o.is_initialized_) {
      ::new (&value_) T(o.value_);
      is_initialized_ = true;
    }

    return *this;
  }

  Maybe(const T& o) {
    static_assert(std::is_copy_constructible<T>::value,
                  "Cannot create copy constructor for a non-copyable T");

    ::new (&value_) T(o);
    is_initialized_ = true;
  }

  ~Maybe() {
    static_assert(std::is_destructible<T>::value,
                  "Maybe<T> can only be destructed for destructable T");
    destroy_self();
  }

  bool operator==(const Maybe<T>& o) const {
    if (is_initialized_ != o.is_initialized_) {
      return false;
    }

    if (is_initialized_) {
      return value_ == o.value_;
    }

    return true;
  }

  bool operator==(const T& o) const {
    if (!is_initialized_) {
      return false;
    }

    return value_ == o;
  }

  bool operator==(const empty_maybe&) const { return !is_initialized_; }

  //////////////////////////////////////////////////////
  // Useful API
  //////////////////////////////////////////////////////
 public:
  bool has_value() const { return is_initialized_; }

  bool is_empty() const { return !has_value(); }

  // DANGEROUS - should not be called without first checking has_value
  T& get() { return value_; }
  const T& get() const { return value_; }

  // DANGEROUS - should not be called without first checking has_value
  T move() {
    is_initialized_ = false;
    return std::move(value_);
  }

  template <typename U>
  Maybe<U> map(std::function<U(T)> fn) {
    static_assert(std::is_copy_constructible<T>::value,
                  "Cannot map on non-copyable T value - use map_move instead");

    if (!is_initialized_) {
      return Maybe<U>();
    }

    return fn(value_);
  }

  /** Invoke a callback with the held value if and only if a value is present */
  void if_present(std::function<void(const T&)> cb) const {
    if (is_initialized_) {
      cb(value_);
    }
  }

  void map(std::function<void(T)> fn) {
    static_assert(std::is_copy_constructible<T>::value,
                  "Cannot map on non-copyable T value - use map_move instead");

    if (!is_initialized_) {
      return;
    }

    fn(value_);
  }

  template <typename U>
  Maybe<U> map_move(std::function<U(T&&)> fn) {
    static_assert(std::is_move_constructible<T>::value,
                  "Cannot map on non-movable T value");

    if (!is_initialized_) {
      return Maybe<U>();
    }

    Maybe<U> rsl = fn(std::move(value_));
    destroy_self();
    return std::move(rsl);
  }

  T or_else(T default_value) {
    static_assert(std::is_copy_constructible<T>::value,
                  "Cannot use or_else on non-copyable T value");
    if (is_initialized_) {
      return value_;
    }

    return default_value;
  }

  template <typename U>
  Maybe<U> flat_map(std::function<Maybe<U>(T)> cb) {
    static_assert(
        std::is_copy_constructible<T>::value,
        "Cannot map on non-copyable T value - use flat_map_move instead");
    if (!is_initialized_) {
      return Maybe<U>();
    }

    auto val = cb(value_);
    if (val.is_empty()) {
      return Maybe<U>();
    }

    return val.value_;
  }

 private:
  union {
    uint8_t dummy_;
    T value_;
  };
  bool is_initialized_;

 private:
  void destroy_self() {
    if (is_initialized_) {
      value_.~T();
      dummy_ = 0x00;
      is_initialized_ = false;
    }
  }
};

template <typename T>
Maybe<T> maybe_from_nullable_ptr(T* ptr) {
  if (ptr == nullptr) {
    return empty_maybe{};
  }

  return *ptr;
}

}  // namespace indigo::core

#endif
