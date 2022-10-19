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
  let qxx = q.x * q.x;
  let qyy = q.y * q.y;
  let qzz = q.z * q.z;
  let qxz = q.x * q.z;
  let qxy = q.x * q.y;
  let qyz = q.y * q.z;
  let qwx = q.w * q.x;
  let qwy = q.w * q.y;
  let qwz = q.w * q.z;

  var tbn: mat3x3<f32>;

  tbn[0][0] = 1.0 - 2.0 * (qyy + qzz);
  tbn[0][1] = 2.0 * (qxy + qwz);
  tbn[0][2] = 2.0 * (qxz - qwy);

  tbn[1][0] = 2.0 * (qxy - qwz);
  tbn[1][1] = 1.0 - 2.0 * (qxx + qzz);
  tbn[1][2] = 2.0 * (qyz + qwx);

  tbn[2][0] = 2.0 * (qxz + qwy);
  tbn[2][1] = 2.0 * (qyz - qwx);
  tbn[2][2] = 1.0 - 2.0 * (qxx + qyy);

  tbn[0] = normalize(tbn[0]);
  tbn[1] = normalize(tbn[1]);
  tbn[2] = normalize(tbn[2]);

  return tbn;
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
  out.normal = (mat_world * vec4<f32>(tbn_from_quat(vertex.tbn_quat)[2], 0.0)).xyz;
  out.albedo = vertex.albedo;
  out.metallic = vertex.metallic;
  out.roughness = vertex.roughness;
  out.ao = vertex.ao;
  out.frag_coord = cameraParams.mat_proj * cameraParams.mat_view * vec4<f32>(out.world_pos, 1.);

  return out;
}
