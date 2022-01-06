#ifndef LIB_IGCORE_DIRTYABLE_H
#define LIB_IGCORE_DIRTYABLE_H

/**
 * Dirtyable - a thing that can be "dirtied" as it is modified, and then
 *  "cleaned" by some caller. Useful as a very primitive form of change
 *  detection.
 */

namespace indigo::core {

/**
 * DirtyablePod - intended for use with POD classes
 */
template <typename T>
class DirtyablePod {
 public:
  DirtyablePod() : data_(), is_dirty_(true) {}

  operator T&() {
    is_dirty_ = true;
    return data_;
  }

  operator const T&() const { return data_; }

  const T& get() const { return data_; }

  bool is_dirty() { return is_dirty_; }

  void clean() { is_dirty_ = false; }

 private:
  T data_;
  bool is_dirty_;
};

}  // namespace indigo::core

#endif
