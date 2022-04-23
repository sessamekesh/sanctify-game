#include "evil_emscripten_hacks.h"

using namespace sanctify;

namespace {
emscripten::val kPreferredTextureJsCallback = emscripten::val::null();
}

wgpu::TextureFormat evil_hacks::get_preferred_texture_format() {
  int jsRsl = ::kPreferredTextureJsCallback().as<int>();
  switch (jsRsl) {
    case 0:
    default:
      return wgpu::TextureFormat::BGRA8Unorm;
    case 1:
      return wgpu::TextureFormat::RGBA8Unorm;
  }
}

void evil_hacks::set_preferred_texture_js_callback(emscripten::val cb) {
  ::kPreferredTextureJsCallback = cb;
}
