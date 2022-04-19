#include "evil_emscripten_hacks.h"

using namespace sanctify;

namespace {
emscripten::val kPreferredTextureJsCallback = {};
}

wgpu::TextureFormat evil_hacks::get_preferred_texture_format() {
  int jsRsl = ::kPreferredTextureJsCallback();
  switch (jsRsl) {
    case 0:
    default:
      return wgpu::TextureFormat::BGRA8Unorm;
    case 1:
      wgpu::TextureFormat::RGBA8Unorm;
  }
}

void evil_hacks::set_preferred_texture_js_callback(emscripten::val cb) {
  ::kPreferredTextureJsCallback = cb;
}
