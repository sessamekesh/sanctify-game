#include "preload_shader.h"

using namespace sanctify;

const std::string sanctify::kPreloadShaderVsSrc =
    R"(@stage(vertex) fn main(@builtin(vertex_index) VI : u32)
-> @builtin(position) vec4<f32> {
  var pos = array<vec2<f32>, 4>(
    vec2<f32>(1., 1.),
    vec2<f32>(1., -1.),
    vec2<f32>(-1., 1.),
    vec2<f32>(-1., -1.));
  return vec4<f32>(pos[VI], 0., 1.);
})";

const std::string sanctify::kPreloadShaderFsSrc = R"(struct FrameParams {
  viewDimensions: vec2<f32>,
  t: f32,
  ballSize: f32,
  pathRadius: f32
}

@group(0) @binding(0) var<uniform> frameParams: FrameParams;

fn sqTriCirPct(t: f32, idx: f32) -> vec2<f32> {
  let x = abs(cos(t * 1. + (t * idx * 0.25)));
  let xx = x * x;
  let y = 1. - xx;
  return vec2(xx, y);
}

fn particleSpeed(idx: f32) -> f32 { return 0.5 + idx * 0.95; }

fn theta(t: f32, idx: f32) -> f32 { return t * particleSpeed(idx) + idx * 6.24; }

fn circlePosition(t: f32, idx: f32, radius: f32) -> vec2<f32> {
  let angle = theta(t, idx);
  return vec2<f32>(cos(angle) * radius, sin(angle) * radius);
}

fn squarePosition(t: f32, idx: f32, radius: f32) -> vec2<f32> {
  let angle = theta(t, idx);
  let sqt = min(abs(1. / cos(angle)), abs(1. / sin(angle)));
  return vec2<f32>(cos(angle) * sqt * radius, sin(angle) * sqt * radius);
}

fn col(t: f32, idx: f32) -> vec3<f32> {
  var b = abs(vec3<f32>(sin(t / 2.), cos(t / 5.), idx * 0.15 + 0.85));
  b = vec3<f32>(b.xy / length(b.xy), b.z);
  return b;
}

@stage(fragment) fn main(@builtin(position) pos: vec4<f32>) -> @location(0) vec4<f32> {
  let t = frameParams.t;
  let ballSize = frameParams.ballSize;
  let pathRadius = frameParams.pathRadius;
  let origin = frameParams.viewDimensions / 2.;
  let fragPos = pos.xy;

  var out = vec4<f32>(0., 0., 0., 1.);

  for (var idx: f32 = 0.; idx < 1.; idx += (1. / 32.)) {
    let circleComponent = circlePosition(t, idx, pathRadius);
    let squareComponent = squarePosition(t, idx, pathRadius);
    let coeffs = sqTriCirPct(t, idx);
    let offset = coeffs.x * circleComponent + coeffs.y * squareComponent;

    if (length((offset + origin) - fragPos) < ballSize) {
      out = out + vec4<f32>(col(t, idx), 0.0);
    }
  }

  return out;
}
)";
