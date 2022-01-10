#ifndef SANCTIFY_GAME_CLIENT_SRC_APP_STARTUP_SCENE_STARTUP_SHADER_SRC_H
#define SANCTIFY_GAME_CLIENT_SRC_APP_STARTUP_SCENE_STARTUP_SHADER_SRC_H

#include <glm/glm.hpp>

namespace sanctify {

const char* kStartupShaderVsSrc =
    R"([[stage(vertex)]] fn main([[builtin(vertex_index)]] VI : u32)
-> [[builtin(position)]] vec4<f32> {
  var pos = array<vec2<f32>, 4>(
    vec2<f32>(-1., -1.),
    vec2<f32>(1., -1.),
    vec2<f32>(1., -1.),
    vec2<f32>(1., 1.));
  return vec4<f32>(pos[VI], 0., 1.);
})";

struct FrameParams {
  glm::vec2 Dimensions;
  float Time;
};

// TODO (sessamekesh): Take inspiration from
// https://www.shadertoy.com/view/WtjBRz
const char* kStartupShaderFsSrc = R"(struct FrameParams {
  Dimensions: vec2<f32>;
  Time: f32;
};

[[group(0), binding(0)]] var<uniform> frameParams: FrameParams;

struct FragIn {
  [[builtin(position)]] FragCoord: vec3<f32>;
};

[[stage(fragment)]] fn main(in: FragIn) -> [[location(0)]] vec4<f32> {
  let coord = frameParams.Dimensions * in.FragCoord.xy;
  let line = (sin(frameParams.Time) + 1.) * 400.;
  if (coord.x > line) {
    return vec4<f32>(1., 0., 0., 1.);
  }
  return vec4<f32>(0., 0., 1., 1.);
})";

}  // namespace sanctify

#endif
