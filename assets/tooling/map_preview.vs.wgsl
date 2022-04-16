struct CameraParamsUbo {
  mat_view: mat4x4<f32>,
  mat_proj: mat4x4<f32>
}

struct ModelVsParams {
  mat_model: mat4x4<f32>
};

@group(0) @binding(0) var<uniform> cameraParams: CameraParamsUbo;
@group(0) @binding(1) var<uniform> modelVsParams: ModelVsParams;

struct VertexIn {
  @location(0) pos: vec3<f32>,
  @location(1) normal: vec3<f32>
}

struct VertexOut {
  @builtin(position) pos: vec4<f32>,
  @location(0) world_pos: vec3<f32>,
  @location(1) normal: vec3<f32>
}

@stage(vertex)
fn main(in: VertexIn) -> VertexOut {
  var out: VertexOut;

  out.world_pos = (modelVsParams.mat_model * vec4(in.pos, 1.)).xyz;
  out.pos = cameraParams.mat_proj * cameraParams.mat_view * vec4<f32>(out.world_pos, 1.);
  out.normal = (modelVsParams.mat_model * vec4(in.normal, 0.)).xyz;

  return out;
}
