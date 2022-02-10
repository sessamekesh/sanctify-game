struct VertexInput {
  @location(0) position: vec3<f32>;
  @location(1) normal: vec3<f32>;
  @location(2) mat_world_0: vec4<f32>;
  @location(3) mat_world_1: vec4<f32>;
  @location(4) mat_world_2: vec4<f32>;
  @location(5) mat_world_3: vec4<f32>;
  @location(6) obj_color: vec3<f32>;
};

struct VertexOutput {
  @builtin(position) frag_coord: vec4<f32>;
  @location(0) world_pos: vec3<f32>;
  @location(1) world_normal: vec3<f32>;
  @location(2) obj_color: vec3<f32>;
};

struct CameraParamsUbo {
  mat_view: mat4x4<f32>;
  mat_proj: mat4x4<f32>;
};

@group(0) @binding(0) var<uniform> cameraParams: CameraParamsUbo;

@stage(vertex)
fn main(vertex: VertexInput) -> VertexOutput {
  var out: VertexOutput;

  let mat_world = mat4x4<f32>(
    vertex.mat_world_0,
    vertex.mat_world_1,
    vertex.mat_world_2,
    vertex.mat_world_3);

  out.world_pos = (mat_world * vec4<f32>(vertex.position, 1.)).xyz;
  out.world_normal = (mat_world * vec4<f32>(vertex.normal, 0.)).xyz;
  out.frag_coord = cameraParams.mat_proj * cameraParams.mat_view * vec4<f32>(out.world_pos, 1.);
  out.obj_color = vertex.obj_color;

  return out;
}
