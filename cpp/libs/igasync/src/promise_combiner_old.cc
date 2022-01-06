#include <igasync/promise_combiner_old.h>

using namespace indigo;
using namespace core;

PromiseCombinerOld::SuccessResult::SuccessResult(
    std::unordered_map<uint32_t, RTIPtr> &&results,
    std::vector<std::shared_ptr<void>> &&lifetime_ptrs)
    : results_(std::move(results)), lifetime_ptrs_(std::move(lifetime_ptrs)) {}

std::shared_ptr<Promise<PromiseCombinerOld::SuccessResult>>
PromiseCombinerOld::build() {
  {
    std::shared_lock l(promises_mutex_);
    if (final_promise_) return final_promise_;
  }

  std::lock_guard l(promises_mutex_);
  final_promise_ = Promise<PromiseCombinerOld::SuccessResult>::create();

  maybe_finish();

  return final_promise_;
}

void PromiseCombinerOld::maybe_finish() {
  // NOTICE: This method should be thread locked - in of itself, this is NOT
  // a thread safe method!
  if (!final_promise_ || final_promise_->is_finished()) {
    return;
  }

  if (!rejected_values_.empty()) {
    final_promise_->resolve(PromiseCombinerOld::SuccessResult(
        std::move(resolved_values_), std::move(lifetime_ptrs_)));
    return;
  }

  if (unresolved_promises_.empty()) {
    final_promise_->resolve(PromiseCombinerOld::SuccessResult(
        std::move(resolved_values_), std::move(lifetime_ptrs_)));
    return;
  }
}