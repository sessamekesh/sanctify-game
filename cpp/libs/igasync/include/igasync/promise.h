#ifndef _LIB_IGASYNC_PROMISE_H_
#define _LIB_IGASYNC_PROMISE_H_

#include <igasync/task_list.h>
#include <igcore/config.h>
#include <igcore/log.h>

#include <memory>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <variant>

namespace indigo::core {

/**
 * Streamlined Promise implementation for Indigo code
 *
 * Loosely modeled off of the JavaScript Promise API. VERY loosely.
 *
 * A Promise is basically an Optional / Maybe type that has two additional
 *  properties and a bunch of built-in behavior that make it very useful for
 *  writing asynchronous code with or without concurrency, with little blocking.
 *
 * (1) A promise value will be set EXACTLY ONCE
 * (2) A promise SHOULD always have a value set eventually (this is the
 *     responsibility of the API user)
 * (3) Once set, the promise WILL ALWAYS have the same value (with the
 *     exception of move semantics - see IMPORTANT NOTE below)
 *
 * Reading code that wants to use the value inside a promise can provide a
 *  callback that eventually be scheduled. If the value is already present,
 *  the callback is scheduled immediately against the given task list. If the
 *  value is not present, it will be scheduled at some point in the future when
 *  the value for the promise is set.
 *
 * IMPORTANT NOTE: This implementation usually keeps ownership of the data it
 *  stores, but provides a "move" semantic. This semantic may be used exactly
 *  once, and should be the last callback registered against the promise.
 *  Attempting to set further listeners or set multiple move listeners will
 *  result in a warning message and will very likely cause ugly bugs.
 *
 * ------------------------------- Usage -------------------------------
 *
 * Promises must always be wrapped in a shared_ptr, create a promise with the
 *  static "Create" command:
 *
 * ```
 * auto my_promise = Promise<int>::create();
 * ```
 *
 * Register callbacks with the "on_success" method. A const auto reference to
 *  the held data will be provided as the only parameter. Pass in a task list
 *  that the callback should be executed against.
 *
 * ```
 * my_promise->on_success(
 *     [](const int& number) { Logger::log() << "Your number is " << number"; },
 *     main_task_list);
 * ```
 *
 * You can also chain promises by providing a transformation. I haven't figured
 *  out the template arguments very much yet, so you might need to be pretty
 *  liberal with those (my bad)
 *
 * ```
 * std::shared_ptr<Promise<std::string>> string_promise =
 *    my_promise->then<std::string>(
 *        [](const int& number) { return std::to_string(number); },
 *        main_task_list);
 * ```
 *
 * Now my_promise has two callbacks attached to it, and the second callback will
 *  populate string_promise with the std::string returned from the callback.
 *
 * Finally, to set the value on the promise, use the "resolve" method.
 *
 * ```
 * my_promise->resolve(42);
 * ```
 *
 * All of these functions are thread-safe.
 */

template <class ValT>
class Promise : public std::enable_shared_from_this<Promise<ValT>> {
 private:
  struct ThenOp {
    std::function<void(const ValT&)> Fn;
    std::string Label;
    std::shared_ptr<TaskList> Executor;
  };

  struct ConsumingThenOp {
    std::function<void(const ValT)> Fn;
    std::string Label;
    std::shared_ptr<TaskList> Executor;
  };

  Promise(std::string label = "")
      : label_(std::move(label)), accepts_thens_(true), remaining_thens_(0) {}

 public:
  inline static std::shared_ptr<Promise<ValT>> create(std::string label = "") {
    return std::shared_ptr<Promise<ValT>>(new Promise<ValT>(label));
  }

  Promise(const Promise<ValT>&) = delete;
  Promise(Promise<ValT>&&) = delete;
  Promise<ValT>& operator=(const Promise<ValT>&) = delete;
  Promise<ValT>& operator=(Promise<ValT>&&) = delete;
  ~Promise() = default;

  /**
   *
   * Standard promise API stuff goes here!!
   *
   */

  /**
   * Analagous to JavaScript Promise.then - invoke callback when this
   * promise resolves. Does not consume the value of the promise.
   */
  Promise<ValT>& on_success(std::function<void(const ValT&)> fn,
                            std::shared_ptr<TaskList> task_list,
                            std::string op_label = "") {
    std::lock_guard l(then_queue_mutex_);
    if (!accepts_thens_) {
      core::Logger::log(kLogLabel)
          << "Cannot add success listener (" << op_label
          << ") - a finalizing callback has been declared on this promise";
      return *this;
    }

    std::shared_lock l2(result_mutex_);
    if (result_.has_value()) {
      task_list->add_task(Task::of(
          [fn = std::move(fn), this, lifetime = this->shared_from_this()]() {
            resolve_inner(std::move(fn));
          }));
      return *this;
    }

    // Promise is still pending in this case - add as a callback
    ThenOp op = {std::move(fn), std::move(op_label), std::move(task_list)};
    {
      std::lock_guard l(rsl_consumption_mutex_);
      remaining_thens_++;
    }
    then_queue_.emplace(std::move(op));

    return *this;
  }

  /**
   * Finalize this promise with a successful result - this will immediately
   * queue up all success callbacks
   */
  void resolve(ValT val) {
    {
      std::scoped_lock l(result_mutex_);

      if (result_.has_value()) {
        core::Logger::err(kLogLabel)
            << "Attempted to resolve an already finished promise";
        return;
      }

      result_ = std::move(val);
      is_finished_ = true;
    }

    // Flush queue of pending operations
    {
      std::scoped_lock l(then_queue_mutex_);
      while (!then_queue_.empty()) {
        auto v = std::move(then_queue_.front());
        then_queue_.pop();
        auto fn = std::move(v.Fn);
        auto task_list = std::move(v.Executor);
        auto op_label = std::move(v.Label);

        task_list->add_task(Task::of(
            [fn = std::move(fn), this, lifetime = this->shared_from_this()]() {
              resolve_inner(std::move(fn));
            }));
      }
    }

    maybe_consume_rsl();
  }

  /**
   * Utility wrapper that ONLY WORKS for the success case - map the result of
   *  this promise to a new promise based on the function provided.
   *
   * Example:
   * Promise<int> p;
   * Promise<long> double = p.then([](const int& v) { return v * 2; }, ctx);
   */
  template <typename MT>
  std::shared_ptr<Promise<MT>> then(std::function<MT(const ValT&)> cb,
                                    std::shared_ptr<TaskList> task_list,
                                    std::string op_label = "") {
    auto tr = Promise<MT>::create(op_label);
    on_success([tr, cb = std::move(cb)](const ValT& v) { tr->resolve(cb(v)); },
               task_list, op_label);
    return tr;
  }

  /**
   * Utility wrapper that does the same thing as "then" (above), but with a
   * function that returns a promise instead of one that returns the value
   * synchronously.
   */
  template <typename MT>
  std::shared_ptr<Promise<MT>> then_chain(
      std::function<std::shared_ptr<core::Promise<MT>>(const ValT&)> cb,
      std::shared_ptr<TaskList> task_list, std::string op_label = "") {
    auto tr = Promise<MT>::create(op_label);
    on_success(
        [tr, cb = std::move(cb), op_label, task_list](const ValT& val) {
          cb(val)->on_success(
              [tr, task_list](const MT& vv) { tr->resolve(vv); }, task_list,
              op_label);
        },
        task_list, op_label);
    return tr;
  }

  /** Create a promise that is immediately resolved with the given value */
  static std::shared_ptr<Promise<ValT>> immediate(ValT&& v) {
    auto tr = create();
    tr->resolve(v);
    return tr;
  }

  /**
   * Final consumption of a promise - like "on_success" but consumes the value
   * of the promise on execution
   */
  Promise<ValT>& consume(std::function<void(ValT&&)> cb,
                         std::shared_ptr<TaskList> task_list,
                         std::string op_label = "") {
    std::lock_guard l(then_queue_mutex_);
    if (!accepts_thens_) {
      core::Logger::log(kLogLabel)
          << "Cannot add consume callback (" << op_label
          << ") - a finalizing callback has been declared on this promise";
      return *this;
    }
    accepts_thens_ = false;

    std::shared_lock l2(result_mutex_);
    if (result_.has_value()) {
      task_list->add_task(Task::of(
          [cb = std::move(cb), this, lifetime = this->shared_from_this()]() {
            cb(std::move(*result_));
          }));
      return *this;
    }
    // Promise is still pending - add as a callback
    ConsumingThenOp op = {std::move(cb), std::move(op_label),
                          std::move(task_list)};
    consuming_then_op_ = std::move(op);

    return *this;
  }

  /**
   * Returns true if the promise has finished (resolved).
   */
  bool is_finished() {
    std::shared_lock l(result_mutex_);
    return is_finished_;
  }

  /** UNSAFE - AVOID UNLESS YOU KNOW WHAT YOU ARE DOING! */
  const ValT& unsafe_sync_get() {
    std::shared_lock m(result_mutex_);
    return *result_;
  }

  /** UNSAFE - AVOID UNLESS YOU KNOW WHAT YOU ARE DOING! */
  ValT&& unsafe_sync_move() { return *(std::move(result_)); }

 private:
  void resolve_inner(const std::function<void(const ValT&)>& fn) {
    // No lock required here - once promise resolves, the result is not
    // changed until it is consumed, and the consumption only happens after
    // all tasks have been executed
    fn(*result_);

    {
      std::lock_guard l(rsl_consumption_mutex_);
      remaining_thens_--;
      maybe_consume_rsl();
    }
  }

  // Possibly consume the final result of the promise and transfer it to the
  // consuming caller. Only resolves if there are no more pending thens.
  void maybe_consume_rsl() {
    std::lock_guard l(then_queue_mutex_);

    if (remaining_thens_ == 0 && consuming_then_op_.has_value()) {
      ConsumingThenOp op = std::move(*consuming_then_op_);
      op.Executor->add_task(Task::of(
          [fn = std::move(op.Fn), this, lifetime = this->shared_from_this()]() {
            fn(std::move(*std::move(result_)));
          }));
      remaining_thens_ = -1;
    }
  }

 private:
  std::string label_;

  std::shared_mutex result_mutex_;
  std::optional<ValT> result_;

  std::mutex then_queue_mutex_;
  std::mutex rsl_consumption_mutex_;
  std::queue<ThenOp> then_queue_;
  int remaining_thens_;
  bool accepts_thens_;
  std::optional<ConsumingThenOp> consuming_then_op_;

  bool is_finished_ = false;

  inline const static std::string kLogLabel = "Promise";
};

struct EmptyPromiseRsl {};

}  // namespace indigo::core

#endif
