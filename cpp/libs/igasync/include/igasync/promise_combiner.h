#ifndef LIB_IGASYNC_PROMISE_COMBINER_H
#define LIB_IGASYNC_PROMISE_COMBINER_H

#include <igasync/promise.h>
#include <igcore/typeid.h>
#include <igcore/vector.h>

#include <atomic>
#include <cassert>
#include <memory>

namespace indigo::core {

/**
 * PromiseCombiner - fills the use case of the JavaScript Promise.all
 *
 * A trivially copyable "PromiseKey" object is returned for each promise added -
 * the result of that promise can be obtained by quering with that key in the
 * result object.
 *
 * Example usage:
 *
 * PromiseCombiner combiner;
 * auto foo_key = combiner.add(foo_promise);
 * auto bar_key = combiner.add(bar_promise);
 *
 * combiner.combine()->on_success([foo_key, bar_key](const auto& result) {
 *   const auto& foo = result.get(foo_key);
 *   const auto& bar = result.get(bar_key);
 *   doSomeSynchronousThing(foo, bar);
 * });
 *
 * The secret sauce behind this:
 * (1) Promises have a synchronous API that can be safely called after the
 *     promise resolves.
 * (2) PromiseKeys are strongly typed, same as the promises themselves, and
 *     cannot be constructed outside of the PromiseCombiner object.
 *
 * At some point in the future, a "race" API may be supported as well.
 */

class PromiseCombiner : public std::enable_shared_from_this<PromiseCombiner> {
 public:
  template <typename T>
  class PromiseCombinerKey {
   public:
    friend class PromiseCombiner;
    ~PromiseCombinerKey() = default;
    PromiseCombinerKey(const PromiseCombinerKey&) = default;
    PromiseCombinerKey& operator=(const PromiseCombinerKey&) = default;
    PromiseCombinerKey(PromiseCombinerKey&&) = default;
    PromiseCombinerKey& operator=(PromiseCombinerKey&&) = default;

    PromiseCombinerKey() = delete;

    uint32_t key() const { return key_; }

   private:
    explicit PromiseCombinerKey(uint32_t key) : key_(key) {}
    uint32_t key_;
  };

 public:
  class PromiseCombinerResult {
   public:
    friend class PromiseCombiner;

    template <typename T>
    const T& get(const PromiseCombinerKey<T>& key) const {
      for (int i = 0; i < combiner_->promise_ptrs_.size(); i++) {
        if (combiner_->promise_ptrs_[i].Key == key.key_) {
          std::shared_ptr<core::Promise<T>> promise_ptr =
              std::static_pointer_cast<core::Promise<T>>(
                  combiner_->promise_ptrs_[i].PromiseRaw);
          if (!promise_ptr) {
            core::Logger::err("PromiseCombinerResult")
                << "Failed pointer cast for some reason";
          }
          return promise_ptr->unsafe_sync_get();
        }
      }

      core::Logger::err("PromiseCombinerResult")
          << "Get value failed: promise with given key not found";
      assert(false);
      // This line keeps the compiler happy, but it's nonsense. The application
      //  is in a very bad state if this line is ever reached.
      assert(false && "Sins against God found in PromiseCombinerResult::get");
      return std::static_pointer_cast<core::Promise<T>>(
                 combiner_->promise_ptrs_[0].PromiseRaw)
          ->unsafe_sync_get();
    }

    template <typename T>
    T&& move(const PromiseCombinerKey<T>& key) const {
      for (int i = 0; i < combiner_->promise_ptrs_.size(); i++) {
        if (combiner_->promise_ptrs_[i].Key == key.key_) {
          std::shared_ptr<core::Promise<T>> promise_ptr =
              std::static_pointer_cast<core::Promise<T>>(
                  combiner_->promise_ptrs_[i].PromiseRaw);
          if (!promise_ptr) {
            core::Logger::err("PromiseCombinerResult")
                << "Failed pointer cast for some reason";
          }
          return promise_ptr->unsafe_sync_move();
        }
      }

      core::Logger::err("PromiseCombinerResult")
          << "Move value failed: promise with given key not found";
      // This line keeps the compiler happy, but it's nonsense. The application
      //  is in a very bad state if this line is ever reached.
      assert(false && "Sins against God found in PromiseCombinerResult::move");
      return std::static_pointer_cast<core::Promise<T>>(
                 combiner_->promise_ptrs_[0].PromiseRaw)
          ->unsafe_sync_move();
    }

   private:
    PromiseCombinerResult(std::shared_ptr<PromiseCombiner> combiner)
        : combiner_(combiner) {}
    std::shared_ptr<PromiseCombiner> combiner_;
  };

 public:
  template <typename T>
  PromiseCombinerKey<T> add(std::shared_ptr<core::Promise<T>> promise,
                            std::shared_ptr<core::TaskList> task_list) {
    std::lock_guard l(rsl_mut_);
    if (is_combine_requested_) {
      core::Logger::err("PromiseCombiner")
          << "New promise added after combiner is finished! This is a bug";
      assert(false);
    }

    PromiseCombinerKey<T> key(next_key_++);
    promise_ptrs_.push_back({key.key_, promise, false});
    promise->on_success(
        [key, l = shared_from_this()](const auto&) {
          // formatting comment
          l->resolve_promise(key.key_);
        },
        task_list);
    return key;
  }

 public:
  static std::shared_ptr<PromiseCombiner> Create();
  PromiseCombiner(const PromiseCombiner&) = delete;
  PromiseCombiner(PromiseCombiner&&) = delete;
  PromiseCombiner& operator=(const PromiseCombiner&) = delete;
  PromiseCombiner& operator=(PromiseCombiner&&) = delete;
  ~PromiseCombiner() = default;

  std::shared_ptr<core::Promise<PromiseCombinerResult>> combine();

 private:
  PromiseCombiner();

  struct PromiseEntry {
    uint32_t Key;
    std::shared_ptr<void> PromiseRaw;
    bool IsResolved;
  };

  void resolve_promise(uint32_t key);

  std::atomic_uint32_t next_key_;

  std::mutex rsl_mut_;
  core::Vector<PromiseEntry> promise_ptrs_;

  std::shared_ptr<core::Promise<PromiseCombinerResult>> final_promise_;
  bool is_combine_requested_;
};

}  // namespace indigo::core

#endif
