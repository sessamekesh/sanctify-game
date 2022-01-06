#ifndef _LIB_IGASYNC_PROMISE_COMBINER_OLD_H_
#define _LIB_IGASYNC_PROMISE_COMBINER_OLD_H_

#include <igasync/promise.h>
#include <igasync/task_list.h>
#include <igcore/typeid.h>

#include <memory>
#include <set>
#include <shared_mutex>
#include <unordered_map>

namespace indigo::core {

/**
 * PromiseCombinerOld - fills the use case of the JavaScript "Promise.all"
 *
 * See promise_combiner.h for an updated implementation that doesn't rely on
 * all this janky intermediate storage garbage, and keeps type data.
 *
 * A trivially copyable "PromiseKey" object is returned for each promise added -
 * the result of that promise can be obtained by quering with that key in the
 * result object.
 *
 * Example usage:
 *
 * PromiseCombiner::Builder combiner;
 * auto foo_key = combiner.add(foo_promise);
 * auto bar_key = combiner.add(bar_promise);
 *
 * combiner.build()->then([=](const auto& result) {
 *   auto* foo = result.get(foo_key);
 *   auto* bar = result.get(bar_key);
 *   doSomeThing(foo, bar);
 * });
 */
class PromiseCombinerOld
    : public std::enable_shared_from_this<PromiseCombinerOld> {
 private:
  /** RTIPtr - contains a reference but no ownership of some returned value */
  struct RTIPtr {
   public:
    uint32_t TypeKey;
    void* Ptr;

    template <typename T>
    T* get() const {
      if (TypeId::of<T>() != TypeKey) {
        Logger::log("PromiseCombiner::RTIPtr")
            << "'get' called with wrong type - expected " << TypeKey << ", got "
            << TypeId::of<T>();
        return nullptr;
      }

      return reinterpret_cast<T*>(Ptr);
    }
  };

 public:
  class PromiseResultKey {
   public:
    explicit PromiseResultKey(uint32_t key) : key_(key) {}
    PromiseResultKey(const PromiseResultKey&) = default;
    PromiseResultKey& operator=(const PromiseResultKey&) = default;
    PromiseResultKey(PromiseResultKey&&) = default;
    PromiseResultKey& operator=(PromiseResultKey&&) = default;
    ~PromiseResultKey() = default;

    PromiseResultKey() = delete;

    uint32_t key() const { return key_; }

   private:
    uint32_t key_;
  };

 public:
  class SuccessResult {
   public:
    SuccessResult(std::unordered_map<uint32_t, RTIPtr>&& results,
                  std::vector<std::shared_ptr<void>>&& lifetime_ptrs);
    SuccessResult() = delete;
    SuccessResult(const SuccessResult&) = delete;
    SuccessResult& operator=(const SuccessResult&) = delete;
    SuccessResult(SuccessResult&&) noexcept = default;
    SuccessResult& operator=(SuccessResult&&) = default;

    template <typename T>
    T* get(const PromiseResultKey& key) const {
      auto it = results_.find(key.key());
      if (it == results_.end()) {
        Logger::log("PromiseCombinerOld::SuccessResult")
            << "Key not found: " << key.key();
        return nullptr;
      }
      return it->second.get<T>();
    }

   private:
    std::unordered_map<uint32_t, RTIPtr> results_;
    // Keep a list of the promises so that they keep ownership while the
    // combiner is alive - this prevents dropping scope prematurely.
    std::vector<std::shared_ptr<void>> lifetime_ptrs_;
  };

 public:
  template <typename RT>
  PromiseResultKey add(const std::shared_ptr<Promise<RT>>& promise,
                       const std::shared_ptr<TaskList>& task_list) {
    PromiseResultKey key(next_key_++);

    {
      std::lock_guard l(promises_mutex_);

      if (final_promise_) {
        Logger::err("PromiseCombiner::add")
            << "Cannot add to a PromiseCombiner once 'build' is called";
        return key;
      }

      lifetime_ptrs_.push_back(promise);
      unresolved_promises_.insert(key.key());
    }

    promise.get()->on_success(
        [ukey{key.key()}, this, lifetime{shared_from_this()}](const auto& rsl) {
          std::lock_guard l(promises_mutex_);
          unresolved_promises_.erase(ukey);
          resolved_values_.insert(
              {ukey, RTIPtr{TypeId::of<RT>(), (void*)&rsl}});
          maybe_finish();
        },
        task_list, "PromiseCombiner::resolve");

    return key;
  }

  std::shared_ptr<Promise<SuccessResult>> build();

  static std::shared_ptr<PromiseCombinerOld> Create() {
    return std::shared_ptr<PromiseCombinerOld>(new PromiseCombinerOld());
  }

 protected:
  void maybe_finish();

  PromiseCombinerOld() : next_key_(1u), final_promise_(nullptr) {}

 private:
  std::atomic_uint32_t next_key_;

  std::shared_mutex promises_mutex_;
  std::set<uint32_t> unresolved_promises_;
  // Keep a list of the promises so that they keep ownership while the combiner
  // is alive - this prevents dropping scope prematurely.
  std::vector<std::shared_ptr<void>> lifetime_ptrs_;
  std::unordered_map<uint32_t, RTIPtr> resolved_values_;
  std::unordered_map<uint32_t, RTIPtr> rejected_values_;

  std::shared_ptr<Promise<SuccessResult>> final_promise_;
};

}  // namespace indigo::core

#endif
