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
      final_result_(nullptr),
      is_combine_requested_(false) {}

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

  final_promise_->resolve(std::move(final_result_));
}