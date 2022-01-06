#include <emscripten/fetch.h>
#include <igplatform/file_promise.h>

using namespace indigo;
using namespace core;

namespace {

struct Request {
  Request(std::shared_ptr<FilePromiseT> l) : fpp(l) {}

  std::shared_ptr<FilePromiseT> fpp;
};

struct RaiiShield {
  ::Request* ptr;

  RaiiShield(::Request* p) : ptr(p) {}
  ~RaiiShield() {
    if (ptr) delete ptr;
  }
};

struct RaiiGuardFetch {
  emscripten_fetch_t* f;

  RaiiGuardFetch(emscripten_fetch_t* ff) : f(ff) {}
  ~RaiiGuardFetch() {
    if (f) {
      emscripten_fetch_close(f);
    }
  }
};

void em_fetch_success(emscripten_fetch_t* fetch) {
  if (!fetch->userData) {
    return;
  }

  ::Request* r = reinterpret_cast<::Request*>(fetch->userData);

  ::RaiiShield rm(r);
  ::RaiiGuardFetch fg(fetch);

  auto data = std::make_shared<RawBuffer>(fetch->numBytes);
  memcpy(data->get(), fetch->data, fetch->numBytes);
  r->fpp->resolve(core::left(std::move(data)));
}

void em_fetch_error(emscripten_fetch_t* fetch) {
  if (!fetch->userData) {
    return;
  }

  ::Request* r = reinterpret_cast<::Request*>(fetch->userData);

  ::RaiiShield rm(r);
  ::RaiiGuardFetch fg(fetch);

  if (fetch->status == 404) {
    r->fpp->resolve(core::right(FileReadError::FileNotFound));
  } else {
    r->fpp->resolve(core::right(FileReadError::FileNotRead));
  }
}

}  // namespace

std::shared_ptr<FilePromiseT> FilePromise::Create(
    const std::string& fileName, const std::shared_ptr<TaskList>& task_list) {
  auto rsl = FilePromiseT::create();

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

  Request* r = new Request(rsl);
  attr.userData = r;
  attr.onsuccess = em_fetch_success;
  attr.onerror = em_fetch_error;

  emscripten_fetch(&attr, fileName.c_str());

  return rsl;
}