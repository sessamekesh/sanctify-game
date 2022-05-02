struct VertexOutput {
  @builtin(position) vert_pos: vec4<f32>,
  @location(0) uv: vec2<f32>
}

@stage(vertex) fn main(@builtin(vertex_index) VI : u32) -> @builtin(position) vec4<f32> {
  var pos = array<vec2<f32>, 4>(
    vec2<f32>(1., 1.),
    vec2<f32>(1., -1.),
    vec2<f32>(-1., 1.),
    vec2<f32>(-1., -1.));
  var uv = pos[VI] / 2. + 0.5;
  uv.y = -uv.y;
  return VertexOutput(vec4<f32>(pos[VI], 0., 1.), uv);
}
