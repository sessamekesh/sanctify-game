#ifndef SANCTIFY_COMMON_SIMPLE_CLIENT_APP_EVIL_EMSCRIPTEN_HACKS_H
#define SANCTIFY_COMMON_SIMPLE_CLIENT_APP_EVIL_EMSCRIPTEN_HACKS_H

/**
 * Evil hacks required to work with Emscripten builds because of bad design
 * decisions early on in the project. None of these are actually required
 * had I been properly aware of what I was doing when I wrote this code.
 */

#include <emscripten/bind.h>
#include <webgpu/webgpu_cpp.h>

namespace sanctify::evil_hacks {

wgpu::TextureFormat get_preferred_texture_format();
void set_preferred_texture_js_callback(emscripten::val cb);

}  // namespace sanctify::evil_hacks

#endif
