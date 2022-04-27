struct VertexInput {
  // Per-vertex attributes
  @location(0) position: vec3<f32>,
  @location(1) tbn_quat: vec4<f32>,

  // Per-instance attributes
  @location(2) mat_world_0: vec4<f32>,
  @location(3) mat_world_1: vec4<f32>,
  @location(4) mat_world_2: vec4<f32>,
  @location(5) albedo: vec3<f32>,
  @location(6) metallic: f32,
  @location(7) roughness: f32,
  @location(8) ao: f32
}

struct VertexOutput {
  @builtin(position) frag_coord: vec4<f32>,

  // Geometry lighting attributes
  @location(0) world_pos: vec3<f32>,
  @location(1) normal: vec3<f32>,

  // Material parameters
  @location(2) albedo: vec3<f32>,
  @location(3) metallic: f32,
  @location(4) roughness: f32,
  @location(5) ao: f32
}

struct CameraParams {
  mat_view: mat4x4<f32>,
  mat_proj: mat4x4<f32>
}

@group(0) @binding(0) var<uniform> cameraParams: CameraParams;

// https://gist.github.com/reinsteam/9e291ed75925eb74d827
// Originally found in Crytek according to comment (I have not verified this)
fn tbn_from_quat(q: vec4<f32>) -> mat3x3<f32> {
  let dq = q + q;
  let wq = (q + q) * q.w;

  let tbn_t_0 = (q.xyz * dq.xxx) + vec3<f32>(-1., 0., 0.);
  let tbn_b_0 = (q.xyz * dq.yyy) + vec3<f32>(0., -.1, 0.);

  let tbn_t_1 = (wq.wzy * vec3<f32>(1., -1., 1.)) + tbn_t_0;
  let tbn_b_1 = (wq.zwx * vec3<f32>(1., 1., -1.)) + tbn_b_0;

  return mat3x3<f32>(tbn_t_1, tbn_b_1, cross(tbn_t_0, tbn_b_0));
}

@stage(vertex)
fn main(vertex: VertexInput) -> VertexOutput {
  var out: VertexOutput;

  let mat_world = mat4x4<f32>(
    vertex.mat_world_0,
    vertex.mat_world_1,
    vertex.mat_world_2,
    vec4<f32>(0., 0., 0., 1.));

  out.world_pos = (mat_world * vec4<f32>(vertex.position, 1.)).xyz;
  out.normal = tbn_from_quat(vertex.tbn_quat)[2];
  out.albedo = vertex.albedo;
  out.metallic = vertex.metallic;
  out.roughness = vertex.roughness;
  out.ao = vertex.ao;
  out.frag_coord = cameraParams.mat_proj * cameraParams.mat_view * vec4<f32>(out.world_pos, 1.);

  return out;
}
