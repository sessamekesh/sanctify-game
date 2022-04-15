#include <igasync/promise.h>

using namespace indigo;
using namespace core;

std::shared_ptr<Promise<EmptyPromiseRsl>> core::immediateEmptyPromise() {
  auto tr = Promise<EmptyPromiseRsl>::create();
  tr->resolve({});
  return tr;
}
