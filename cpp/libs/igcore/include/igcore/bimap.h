#ifndef LIB_IGCORE_BIMAP_H
#define LIB_IGCORE_BIMAP_H

/**
 * Bi-directional map - values pairs are stored, and lookups may be done against
 * either side of the pair.
 *
 * This is the appropriate data structure for 1:1 relationships between objects.
 *
 * Requirements:
 * - Each side (LT, RT) must have a "less" comparison defined (this can be
 *   overridden in the template paramters as LTLess and RTLess.
 * - Each side must be trivially copyable
 */

#include <igcore/maybe.h>
#include <igcore/vector.h>

#include <map>
#include <set>

namespace indigo::core {

template <typename LT, typename RT, typename LTLess = std::less<LT>,
          typename RTLess = std::less<RT>>
class Bimap {
 public:
  using entry_t = std::tuple<LT, RT>;

  class iterator_t {
   public:
    friend class literator_t;
    friend class riterator_t;

    iterator_t(const Bimap* m, uint32_t idx) : m_(m), idx_(idx) {}

    const entry_t& operator*() const { return m_->entries_[idx_]; }
    const entry_t* operator->() const { return &m_->entries_[idx_]; }
    iterator_t& operator++() {
      idx_++;
      return *this;
    }

    friend bool operator==(const iterator_t& a, const iterator_t& b) {
      return a.m_ == b.m_ && a.idx_ == b.idx_;
    }
    friend bool operator!=(const iterator_t& a, const iterator_t& b) {
      return a.m_ != b.m_ || a.idx_ != b.idx_;
    }

    // private:
    const Bimap* m_;
    uint32_t idx_;
  };

  class literator_t {
   public:
    literator_t(const Bimap* m, uint32_t idx) : m_(m), idx_(idx) {}
    literator_t(iterator_t it) : m_(it.m_), idx_(it.idx_) {}

    LT& operator*() const { return std::get<0>(m_->entries_[idx_]); }
    const LT& operator*() { return std::get<0>(m_->entries_[idx_]); }

    LT* operator->() const { return &std::get<0>(m_->entries_[idx_]); }
    literator_t& operator++() {
      idx_++;
      return *this;
    }

    friend bool operator==(const literator_t& a, const literator_t& b) {
      return a.m_ == b.m_ && a.idx_ == b.idx_;
    }
    friend bool operator!=(const literator_t& a, const literator_t& b) {
      return a.m_ != b.m_ || a.idx_ != b.idx_;
    }
    friend bool operator==(const literator_t& a, const iterator_t& b) {
      return a.m_ == b.m_ && a.idx_ == b.idx_;
    }
    friend bool operator!=(const literator_t& a, const iterator_t& b) {
      return a.m_ != b.m_ || a.idx_ != b.idx_;
    }
    friend bool operator==(const iterator_t& a, const literator_t& b) {
      return a.m_ == b.m_ && a.idx_ == b.idx_;
    }
    friend bool operator!=(const iterator_t& a, const literator_t& b) {
      return a.m_ != b.m_ || a.idx_ != b.idx_;
    }

    friend class riterator_t;
    friend class iterator_t;

    // private:
    const Bimap* m_;
    uint32_t idx_;
  };

  class riterator_t {
   public:
    friend class literator_t;
    friend class iterator_t;

    riterator_t(const Bimap* m, uint32_t idx) : m_(m), idx_(idx) {}
    riterator_t(iterator_t it) : m_(it.m_), idx_(it.idx_) {}

    RT& operator*() const { return std::get<1>(m_->entries_[idx_]); }
    const RT& operator*() { return std::get<1>(m_->entries_[idx_]); }
    RT* operator->() const { return &std::get<1>(m_->entries_[idx_]); }
    riterator_t& operator++() {
      idx_++;
      return *this;
    }

    friend bool operator==(const riterator_t& a, const riterator_t& b) {
      return a.m_ == b.m_ && a.idx_ == b.idx_;
    }
    friend bool operator!=(const riterator_t& a, const riterator_t& b) {
      return a.m_ != b.m_ || a.idx_ != b.idx_;
    }
    friend bool operator==(const riterator_t& a, const iterator_t& b) {
      return a.m_ == b.m_ && a.idx_ == b.idx_;
    }
    friend bool operator!=(const riterator_t& a, const iterator_t& b) {
      return a.m_ != b.m_ || a.idx_ != b.idx_;
    }
    friend bool operator==(const iterator_t& a, const riterator_t& b) {
      return a.m_ == b.m_ && a.idx_ == b.idx_;
    }
    friend bool operator!=(const iterator_t& a, const riterator_t& b) {
      return a.m_ != b.m_ || a.idx_ != b.idx_;
    }

    // private:
    const Bimap* m_;
    uint32_t idx_;
  };

  Bimap() {}

  riterator_t find_l(const LT& key) const {
    auto ltit = l_indices_.find(key);
    if (ltit == l_indices_.end()) {
      return end();
    }

    uint32_t idx = ltit->second;
    const entry_t& e = entries_[idx];
    return riterator_t{this, idx};
  }

  literator_t find_r(const RT& key) const {
    auto rtit = r_indices_.find(key);
    if (rtit == r_indices_.end()) {
      return end();
    }

    uint32_t idx = rtit->second;
    const entry_t& e = entries_[idx];
    return literator_t{this, idx};
  }

  // Perform an insertion if there is no conflict - otherwise do nothing.
  // Function returns the number of values that were inserted into the bimap (0
  // or 1)
  size_t insert(LT l, RT r) {
    auto ltit = l_indices_.find(l);
    if (ltit != l_indices_.end()) {
      return 0;
    }

    auto rtit = r_indices_.find(r);
    if (rtit != r_indices_.end()) {
      return 0;
    }

    auto idx = (uint32_t)entries_.size();
    entries_.push_back({l, r});
    l_indices_.emplace(l, idx);
    r_indices_.emplace(r, idx);
    return 1;
  }

  size_t size() { return entries_.size(); }

  // Erase an entry based on the left hand value - returns the number of erased
  // entries
  size_t erase_l(const LT& key) {
    auto lit = l_indices_.find(key);
    if (lit == l_indices_.end()) {
      return 0;
    }

    return erase_at(lit->second);
  }

  // Erase an entry based on the right hand value - returns the number of erased
  // entries.
  size_t erase_r(const RT& key) {
    auto rit = r_indices_.find(key);
    if (rit == r_indices_.end()) {
      return 0;
    }

    return erase_at(rit->second);
  }

  // Perform an insertion if there is no conflict, or update the existing value
  // if there is an update.
  void insert_or_update(LT l, RT r) {
    auto lit = l_indices_.find(l);
    auto rit = r_indices_.find(r);

    if (lit == l_indices_.end() && rit == r_indices_.end()) {
      insert(l, r);
      return;
    }

    if (lit != l_indices_.end() && rit != r_indices_.end()) {
      if (lit->second == rit->second) {
        // Happy case - this is an already existing entry!
        return;
      }

      // Sad case - there are two separate entries! Erase both, and re-insert.
      erase_l(l);
      erase_r(r);
      insert(l, r);
      return;
    }

    if (lit == l_indices_.end()) {
      uint32_t idx = rit->second;
      entry_t& entry = entries_[idx];
      LT& old_lval = std::get<0>(entry);
      auto old_lit = l_indices_.find(old_lval);
      l_indices_.erase(old_lit);
      l_indices_.emplace(l, idx);
      old_lval = l;
    } else {
      uint32_t idx = lit->second;
      entry_t& entry = entries_[idx];
      RT& old_rval = std::get<1>(entry);
      auto old_rit = r_indices_.find(old_rval);
      r_indices_.erase(old_rit);
      r_indices_.emplace(r, idx);
      old_rval = r;
    }
  }

  iterator_t begin() const { return iterator_t(this, 0); }
  iterator_t end() const { return iterator_t(this, (int)entries_.size()); }

  struct literable {
    const Bimap* m_;

    literator_t begin() const { return m_->begin(); }
    literator_t end() const { return m_->end(); }
  };

  struct riterable {
    const Bimap* m_;

    riterator_t begin() const { return m_->begin(); }
    riterator_t end() const { return m_->end(); }
  };

  literable left_values() const { return literable{this}; }
  riterable right_values() const { return riterable{this}; }

 private:
  size_t erase_at(uint32_t idx) {
    // TODO (sessamekesh): Since ordering is not important, do this to not
    // maintain order
    for (int i = idx + 1; i < entries_.size(); i++) {
      entry_t& e = entries_[i];
      l_indices_[std::get<0>(e)]--;
      r_indices_[std::get<1>(e)]--;
    }
    l_indices_.erase(std::get<0>(entries_[idx]));
    r_indices_.erase(std::get<1>(entries_[idx]));
    entries_.delete_at(idx, true);

    return 1;
  }

  Vector<entry_t> entries_;
  std::map<LT, uint32_t, LTLess> l_indices_;
  std::map<RT, uint32_t, RTLess> r_indices_;
};

}  // namespace indigo::core

#endif
