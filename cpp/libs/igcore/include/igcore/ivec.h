#ifndef _LIB_IGCORE_IVEC_H_
#define _LIB_IGCORE_IVEC_H_

#include <cstdint>
#include <memory>
#include <vector>

namespace indigo::core {

/**
 * Base class for Indigo vector list iterables (analagous to std::vector).
 * Somewhat primitive and focused on the specific needs of Indigo applications.
 *
 * std::vector is more or less unusable for large data sets in WebAssembly
 * applications because it has a slow destruction time which seems to scale
 * O(n) on size n. I'm not sure why this is the case, but I have shown the
 * results experimentally.
 *
 * Indigo lists have a couple utility methods that are nicer to deal with than
 * the STL equivalents too, so prefer using these where convenient for core
 * features. std::vector is not explicitly forbidden, but should be scrutinized.
 *
 * See: https://github.com/sessamekesh/slow-wasm-vec-dtor
 *
 * -------------------------- IMPLEMENTATION NOTES ---------------------------
 * - Underlying data can be assumed to be tightly packed. Implementors need to
 *   respect this property.
 *   + &vec[i + 1] - &vec[i] == sizeof(T)
 *
 * - Pointers to underlying data may NOT be considered stable. Implementations
 *   may re-allocate and move the underlying data, or re-order the underlying
 *   data as it sees fit.
 *
 * - These vectors are NOT thread safe! Callers are responsible for performing
 *   their own locking against the data held within these structures.
 */

template <typename T>
class IVec {
 public:
  IVec(size_t size, size_t capacity) : size_(size), capacity_(capacity) {}

  // Size in bytes of the underlying data set
  size_t raw_size() const { return sizeof(T) * size_; }

  // Count of elements contained in the underlying data set
  size_t size() const { return size_; }

  // Access an element by mutable reference at the provided index
  virtual T& operator[](size_t i) = 0;

  // Access an element by immutable reference at the provided index
  virtual const T& operator[](size_t i) const = 0;

  /**
   * Delete the element at the given index, if in bounds. Returns "true"
   * if an element was deleted at that spot, false if the given index is
   * out of bounds.
   *
   * If ordering is unimportant, set preserve_order to false - this may
   * cause the underlying data set to become unordered, but is much faster.
   *
   * @param i The index in the data set at which to delete an element
   * @param preserve_order Keep the ordering of elements before and after the
   *                       deleted element if true.
   * @return true if an element was deleted, false otherwise (i.e. i was out of
   *         bounds)
   */
  virtual bool delete_at(size_t i, bool preserve_order = true) = 0;

  /**
   * Delete all elements matching the provided value. Uses operator==() to
   * determine equality. Returns the number of elements that were erased.
   *
   * @param val Underlying elements will be erased if they pass operator==()
   * @param preserve_order Keep the ordering of elements before and after the
   *                       deleted element if true (set to false for fast
   *                       deletes if ordering is unimportant)
   * @return the number of elements that were deleted
   */
  size_t erase(const T& val, bool preserve_order = true) {
    size_t deleted = 0u;
    for (size_t i = 0; i < size(); i++) {
      if (operator[](i) == val) {
        delete_at(i, preserve_order);
        deleted++;
      }
    }
    return deleted;
  }

  // Make sure that the underlying data set is at least {new_size} large.
  void reserve(size_t new_size) { alloc_to(new_size); }

  /**Add an element to the back of this array. Data is copied by value.*/
  void push_back(const T& data) {
    alloc_to(++size_);
    operator[](size_ - 1) = data;
  }

  template <typename... Ts>
  void push_back(const T& data, Ts... args) {
    push_back(data);
    push_back(args...);
  }

  /** Add an element to the back of the array. Data is copied by move. */
  void push_back(T&& data) {
    alloc_to(++size_);
    operator[](size_ - 1) = std::move(data);
  }

  /** Get the last element in the group */
  T& last() { return operator[](size_ - 1); }

  /** Get the last element in the group */
  const T& last() const { return operator[](size_ - 1); }

  /**
   * Append the contents of another vector onto this one. Elements are copied by
   * value.
   */
  void append(const IVec<T>& data) {
    for (size_t i = 0; i < data.size(); i++) {
      push_back(data[i]);
    }
  }

  /**
   * True if the vector contains an entry with the given value. Requires type T
   * to have operator== defined.
   *
   * @param val The value to search for
   * @param is_sorted True if the underlying vector can be assumed to be sorted,
   *                  false otherwise. Searching is much faster if this can be
   *                  assumed.
   */
  bool contains(const T& val, bool is_sorted = false) const {
    if (is_sorted) {
      size_t l = 0;
      size_t r = size() - 1;
      while (l <= r) {
        size_t m = l + (r - l) / 2;
        if (operator[](m) == val) {
          return true;
        }

        if (operator[](m) < val) {
          l = m + 1;
        } else {
          r = m - 1;
        }
      }

      return false;
    }

    const size_t len = size();
    for (size_t i = 0; i < len; i++) {
      if (operator[](i) == val) {
        return true;
      }
    }
    return false;
  }

  /**
   * In-place sort the underlying data. Requires type T to have operator== and
   * operator< defined.
   */
  void in_place_sort() {
    if (size() == 0) return;
    T& f = operator[](0);
    ivec_qsort(&f, 0, size() - 1);
  }

  /**
   * @codesmell This method is considered a code smell in production code -
   *            avoid using unless using with an API that absolutely requires a
   *            std::vector. In those cases, consider using a std::vector for
   *            the underlying data set instead.
   *
   * Copy the data from this vector into a std::vector. This performs a full
   * copy and also uses a data structure that performs poorly in WebAssembly.
   *
   * Example of acceptable usage:
   * (1) Underlying data should be in IVec for the common/happy case, but an
   *     uncommon case requires calling into an API that uses std::vector
   * (2) Unit tests that use vector matchers (e.g., UnorderedElementsAre)
   * (3) Non-production use (e.g., editors, developer tools, etc)
   */
  std::vector<T> copy_to_stupid_slow_std_vector() {
    std::vector<T> tr;
    tr.reserve(size());

    for (size_t i = 0; i < size(); i++) {
      tr.push_back(operator[](i));
    }

    return tr;
  }

 protected:
  // Allocate the underlying data set to be able to hold at least size elements
  virtual void alloc_to(size_t size) = 0;

  // Given pointers to two elements, swap their data. The mechanism of doing
  // this depends on the instantiation.
  virtual void swap(T*, T*) = 0;

  // Number of elements in the underlying data set
  size_t size_;

  // Capacity of the underlying data set
  size_t capacity_;

 private:
  void ivec_qsort(T* data, int low, int high) {
    if (low < high) {
      auto pi = ivec_partition(data, low, high);
      ivec_qsort(data, low, pi - 1);
      ivec_qsort(data, pi + 1, high);
    }
  }

  int ivec_partition(T* data, int low, int high) {
    auto& pivot = data[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
      if (data[j] < pivot) {
        i++;
        swap(&data[i], &data[j]);
      }
    }

    swap(&data[i + 1], &data[high]);
    return i + 1;
  }
};

}  // namespace indigo::core

#endif
