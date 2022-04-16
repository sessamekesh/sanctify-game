struct CameraParamsUbo {
  mat_view: mat4x4<f32>,
  mat_proj: mat4x4<f32>
}

@group(0) @binding(0) var<uniform> cameraParams: CameraParamsUbo;

@stage(vertex)
fn main(@location(0) pos: vec3<f32>) -> @builtin(position) vec4<f32> {
  return cameraParams.mat_proj * cameraParams.mat_view * vec4(pos, 1.);
}
