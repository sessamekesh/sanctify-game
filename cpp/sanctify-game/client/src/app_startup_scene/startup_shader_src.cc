#include "startup_shader_src.h"

const char* sanctify::kStartupShaderVsSrc =
    R"(@stage(vertex) fn main(@builtin(vertex_index) VI : u32)
-> @builtin(position) vec4<f32> {
  var pos = array<vec2<f32>, 4>(
    vec2<f32>(1., 1.),
    vec2<f32>(1., -1.),
    vec2<f32>(-1., 1.),
    vec2<f32>(-1., -1.));
  return vec4<f32>(pos[VI], 0., 1.);
})";

const char* sanctify::kStartupShaderFsSrc = R"(struct FrameParams {
  Dimensions: vec2<f32>,
  Time: f32
}

@group(0) @binding(0) var<uniform> frameParams: FrameParams;

// https://www.iquilezles.org/www/articles/smin/smin.htm
fn smin(a: f32, b: f32, k: f32) -> f32 {
  let res = exp2(-k*a)+exp2(-k*b);
  return -log2(res)/k;
}

fn glow(size: f32, dist: f32, falloff: f32) -> f32 {
  return max(size - dist, 0.) / falloff;
}

fn ballPos(time: f32, freq: vec2<f32>, phase: vec2<f32>, radius: f32,
    spin_radius: f32, spin_freq: f32, spin_phase: f32) -> vec2<f32> {
  let wobble = vec2<f32>(
    cos(time * freq.x + phase.x),
    sin(time * freq.y + phase.y)) * radius;
  let spin = vec2<f32>(
    cos(time * spin_freq + spin_phase),
    sin(time * spin_freq + spin_phase)) * spin_radius;

  return wobble + spin;
}

struct FragIn {
  @builtin(position) FragCoord: vec4<f32>
}

@stage(fragment) fn main(in: FragIn) -> @location(0) vec4<f32> {
  let center = frameParams.Dimensions / 2.;
  let t = frameParams.Time;
  let fragCoord = in.FragCoord.xy;

  let ball_pos_0 = center + ballPos(t, vec2<f32>(1.5, 1.87), vec2<f32>(0.1, 0.5), 125., 0., 1., 0.);
  let ball_pos_1 = center + ballPos(t, vec2<f32>(1.2, 3.7), vec2<f32>(0.1, 0.5), 70., 70., 0.125, 0.);
  let ball_pos_2 = center + ballPos(t, vec2<f32>(1.5, 1.87), vec2<f32>(1., 2.), 75., 35., 0.125, 0.5);
  let ball_pos_3 = center + ballPos(t, vec2<f32>(1.5, 1.87), vec2<f32>(1., 2.), 2., 45., 0.2125, -0.5);

  var dist = distance(fragCoord, ball_pos_0);
  var min_dist = distance(fragCoord, ball_pos_0);
  var final_color = vec3<f32>(1., 0., 0.) * clamp(1. - dist / 200., 0., 1.);

  dist = distance(fragCoord, ball_pos_1);
  min_dist = smin(min_dist, dist, 1. / 30.);
  final_color = final_color + vec3<f32>(0., 1., 0.) * clamp(1. - dist / 200., 0., 1.);

  dist = distance(fragCoord, ball_pos_2);
  min_dist = smin(min_dist, dist, 1. / 30.);
  final_color = final_color + vec3<f32>(0., 0., 1.) * clamp(1. - dist / 200., 0., 1.);

  dist = distance(fragCoord, ball_pos_3);
  min_dist = smin(min_dist, dist, 1. / 30.);
  final_color = final_color + vec3<f32>(0.12, 0.23, 0.34) * clamp(1. - dist / 200., 0., 1.);

  return vec4<f32>(final_color * glow(100., min_dist, 20.), 1.);
})";
