#ifndef SANCTIFY_COMMON_UTIL_RESOURCE_REGISTRY_H
#define SANCTIFY_COMMON_UTIL_RESOURCE_REGISTRY_H

#include <igcore/maybe.h>
#include <igcore/pod_vector.h>
#include <igcore/vector.h>

namespace sanctify {

/**
 * Registry of resources that may be set exactly once and will not be
 * modified/deleted over the course of using the registry. Useful for large
 * resources that should not be copied around often.
 *
 * This object has NO synchronization! Perform all mutations on the main thread,
 * and only during times where no reads are being performed on side threads!
 *
 * Also - notice this does NOT clean up memory, please destruct the entire
 * object when the resource registry may be bloated (e.g., between scenes) and
 * only use the registry for long-lived objects
 *
 * Intended for use in ECS systems where heavy CPU and/or GPU objects are used -
 * use should be short-lived (e.g. no persistant references).
 */
template <typename T>
class ReadonlyResourceRegistry {
 public:
  class Key {
   public:
    Key() : key_(0xFFFFFFFFu), index_(0u) {}
    Key(const Key&) = default;
    Key& operator=(const Key&) = default;
    Key(Key&&) = default;
    Key& operator=(Key&&) = default;
    ~Key() = default;

    // Careful using this directly
    uint32_t get_raw_key() const { return key_; }

    bool operator==(const Key& o) { return ~key_ && (key_ == o.key_); }

    friend class ReadonlyResourceRegistry;

   private:
    Key(uint32_t k, uint32_t index) : key_(k), index_(index) {}
    uint32_t key_;
    uint32_t index_;
  };

  ReadonlyResourceRegistry() : next_key_(1ull) {}
  ReadonlyResourceRegistry(const ReadonlyResourceRegistry<T>&) = delete;
  ReadonlyResourceRegistry& operator=(const ReadonlyResourceRegistry<T>&) =
      delete;
  ReadonlyResourceRegistry(ReadonlyResourceRegistry<T>&&) = default;
  ReadonlyResourceRegistry& operator=(ReadonlyResourceRegistry<T>&&) = default;

  Key reserve_resource() {
    uint32_t next_key = next_key_++;

    if (empty_resource_slots_.size() > 0) {
      uint32_t next_index = empty_resource_slots_[0];
      empty_resource_slots_.erase(0, false);

      resources_[next_index] =
          ResourceKeyPair{next_key, indigo::core::empty_maybe{}};
      return Key(next_key, next_index);
    }

    uint32_t next_index = resources_.size();
    resources_.push_back(
        ResourceKeyPair{next_key, indigo::core::empty_maybe{}});
    return Key(next_key, next_index);
  }

  bool set_reserved_resource(Key key, T resource) {
    if (key.index_ >= resources_.size() ||
        resources_[key.index_].key != key.key_ ||
        resources_[key.index_].resource.has_value()) {
      return false;
    }

    resources_[key.index_].resource = std::move(resource);
    return true;
  }

  Key add_resource(T resource) {
    Key key = reserve_resource();
    resources_[key.index_].resource = std::move(resource);
    return key;
  }

  const T* get(Key key) const {
    if (key.index_ >= resources_.size() ||
        resources_[key.index_].key != key.key_ ||
        resources_[key.index_].resource.is_empty()) {
      return nullptr;
    }

    const T& t = resources_[key.index_].resource.get();
    return &t;
  }

  void remove_resource(Key key) {
    if (key.index_ >= resources_.size() ||
        resources_[key.index_].key != key.key_ ||
        resources_[key.index_].resource.is_empty()) {
      return;
    }

    resources_[key.index_].resource = indigo::core::empty_maybe{};
    empty_resource_slots_.push_back(key.index_);
  }

 private:
  struct ResourceKeyPair {
    uint32_t key;
    indigo::core::Maybe<T> resource;
  };

  indigo::core::Vector<ResourceKeyPair> resources_;
  indigo::core::PodVector<uint32_t> empty_resource_slots_;
  uint32_t next_key_;
};

}  // namespace sanctify

#endif
