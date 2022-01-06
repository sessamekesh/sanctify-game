#include <igasync/promise_combiner.h>

using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "PromiseCombiner";
}

std::shared_ptr<PromiseCombiner> PromiseCombiner::Create() {
  return std::shared_ptr<PromiseCombiner>(new PromiseCombiner());
}

PromiseCombiner::PromiseCombiner()
    : next_key_(1u),
      final_promise_(Promise<PromiseCombinerResult>::create()),
      is_combine_requested_(false) {}

std::shared_ptr<core::Promise<PromiseCombiner::PromiseCombinerResult>>
PromiseCombiner::combine() {
  {
    std::lock_guard l(rsl_mut_);
    if (is_combine_requested_) {
      core::Logger::err(kLogLabel) << "Combine requested a second time - this "
                                      "may indicate a programmer error";
      assert(false);
    }

    is_combine_requested_ = true;
  }

  resolve_promise(0u);

  return final_promise_;
}

void PromiseCombiner::resolve_promise(uint32_t key) {
  std::lock_guard l(rsl_mut_);

  if (key != 0u) {
    for (int i = 0; i < promise_ptrs_.size(); i++) {
      if (promise_ptrs_[i].Key == key) {
        promise_ptrs_[i].IsResolved = true;
        break;
      }
    }
  }

  if (!is_combine_requested_) {
    return;
  }

  for (int i = 0; i < promise_ptrs_.size(); i++) {
    if (!promise_ptrs_[i].IsResolved) {
      return;
    }
  }

  final_promise_->resolve(PromiseCombinerResult(shared_from_this()));
}