#ifndef _LIB_IGCORE_EITHER_H_
#define _LIB_IGCORE_EITHER_H_

#include <igcore/maybe.h>

/**
 * Implementation of an Either<R, E> type for Indigo uses
 *
 * An Either<R, E> can either hold a Left<R> or Right<E>, commonly used	for
 *  possibly returning a success (left) or failure (right) state.
 */

namespace indigo::core {

template <typename T>
struct Left {
  T value;
};

template <typename T>
Left<T> left(T const& x) {
  return Left<T>{x};
}

template <typename T>
Left<T> left(T&& x) {
  return {std::move(x)};
}

template <typename T>
struct Right {
  T value;
};

template <typename T>
constexpr Right<T> right(T const& x) {
  return {x};
}

template <typename T>
Right<T> right(T&& x) {
  return {std::move(x)};
}

template <typename LT, typename RT>
class Either {
 public:
  Either() = delete;

  Either(const Either<LT, RT>& o) {
    static_assert(std::is_copy_constructible<LT>::value &&
                      std::is_copy_assignable<RT>::value,
                  "Either<LT, RT> copy constructor is only defined on copyable "
                  "LT and RT");
    if (o.is_left()) {
      ::new (&left_) LT(o.left_);
      is_left_ = true;
    } else {
      ::new (&right_) RT(o.right_);
      is_left_ = false;
    }
  }

  Either(Either<LT, RT>&& o) noexcept {
    if (o.is_left_) {
      ::new (&left_) LT(std::move(o.left_));
      is_left_ = true;
    } else {
      ::new (&right_) RT(std::move(o.right_));
      is_left_ = false;
    }
  }

  Either<LT, RT>& operator=(const Either<LT, RT>& o) {
    static_assert(std::is_copy_assignable<LT>::value &&
                      std::is_copy_assignable<RT>::value,
                  "Either<LT, RT> assignment operator is only defined on "
                  "copy-assignable LT and RT");
    if (o.is_left()) {
      left_ = o.left_;
      is_left_ = true;
    } else {
      right_ = o.right_;
      is_left_ = false;
    }
    return *this;
  }

  Either<LT, RT>& operator=(Either<LT, RT>&& o) noexcept {
    if (o.is_left_) {
      left_ = std::move(o.left_);
      is_left_ = true;
    } else {
      right_ = std::move(o.right_);
      is_left_ = false;
    }
    return *this;
  }

  ~Either() {
    if (is_left_) {
      left_.~LT();
    } else {
      right_.~RT();
    }
  }

  // copy
  Either(const Left<LT>& left) : is_left_(true), left_(left.value) {
    static_assert(
        std::is_copy_constructible<LT>::value,
        "Cannot create a left constructor for a non-copyable Left<T>");
  }

  Either(const Right<RT>& right) : is_left_(false), right_(right.value) {
    static_assert(
        std::is_copy_constructible<RT>::value,
        "Cannot create a right constructor for a non-copyable Right<T>");
  }

  // move
  Either(Left<LT>&& left) : is_left_(true), left_(std::move(left.value)) {
    static_assert(std::is_move_constructible<LT>::value,
                  "Cannot create a left constructor for a non-movable Left<T>");
  }

  Either(Right<RT>&& right) : is_left_(false), right_(std::move(right.value)) {
    static_assert(
        std::is_move_constructible<RT>::value,
        "Cannot create a right constructor for a non-movable Right<T>");
  }

  ////////////////////////////////
  // Public API
  ////////////////////////////////
 public:
  bool is_left() const { return is_left_; }
  bool is_right() const { return !is_left(); }

  Maybe<LT> maybe_left() const {
    static_assert(std::is_copy_constructible<LT>::value,
                  "Cannot maybe_left on a non-copyable LT");
    if (is_left_) {
      return Maybe<LT>(left_);
    }

    return Maybe<LT>::empty();
  }

  Maybe<RT> maybe_right() const {
    static_assert(std::is_copy_assignable<RT>::value,
                  "Cannot maybe_right on a non-copyable RT");

    if (is_left_) {
      return Maybe<RT>::empty();
    }
    return Maybe<RT>(right_);
  }

  Maybe<LT> maybe_left_move() {
    if (is_left_) {
      return Maybe<LT>(std::move(left_));
    }
    return Maybe<LT>::empty();
  }

  Maybe<RT> maybe_right_move() {
    if (!is_left_) {
      return Maybe<RT>(std::move(right_));
    }
    return Maybe<RT>::empty();
  }

  // DANGEROUS - should not be called without first checking is_left
  const LT& get_left() const { return left_; }

  // DANGEROUS - should not be called without first checking is_right
  const RT& get_right() const { return right_; }

  // DANGEROUS - should not be called without first checking is_left
  LT left_move() { return std::move(left_); }

  // DANGEROUS - should not be called without first checking is_right
  RT right_move() { return std::move(right_); }

  template <typename U>
  Either<U, RT> map_left(std::function<U(LT)> fn) const {
    static_assert(
        std::is_copy_constructible<LT>::value &&
            std::is_copy_constructible<RT>::value,
        "Cannot map on non-copyable LT + RT value - use map_left_move instead");

    if (!is_left_) {
      return right<RT>(right_);
    }

    return left<U>(fn(left_));
  }

  template <typename U>
  Either<LT, U> map_right(std::function<U(RT)> fn) const {
    static_assert(std::is_copy_constructible<LT>::value &&
                      std::is_copy_constructible<RT>::value,
                  "Cannot map on non-copyable LT + RT value - use "
                  "map_right_move instead");

    if (is_left_) {
      return left<LT>(left_);
    }

    return right<U>(fn(right_));
  }

 private:
  union {
    LT left_;
    RT right_;
  };
  bool is_left_;
};

}  // namespace indigo::core

#endif
