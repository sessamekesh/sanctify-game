struct GridParams {
  gridline_color: vec3<f32>;
};

@group(1) @binding(0) var<uniform> grid_params: GridParams;

@stage(fragment)
fn main() -> @location(0) vec4<f32> {
  return vec4<f32>(grid_params.gridline_color, 1.);
}
