struct VertexInput {
  [[location(0)]] position: vec3<f32>;
  [[location(1)]] tbn_quat: vec4<f32>;
  [[location(2)]] mat_world_0: vec4<f32>;
  [[location(3)]] mat_world_1: vec4<f32>;
  [[location(4)]] mat_world_2: vec4<f32>;
  [[location(5)]] mat_world_3: vec4<f32>;
  [[location(6)]] bone_weights: vec4<f32>;
  [[location(7)]] bone_indices: vec4<u32>;
};

struct VertexOutput {
  [[builtin(position)]] frag_coord: vec4<f32>;
  [[location(0)]] world_pos: vec3<f32>;
  [[location(1)]] world_normal: vec3<f32>;
};

fn normal_from_quat(q: vec4<f32>) -> vec3<f32> {
  let qxx = q.x * q.x;
  let qyy = q.y * q.y;
  let qzz = q.z * q.z;
  let qxz = q.x * q.z;
  let qxy = q.x * q.y;
  let qyz = q.y * q.z;
  let qwx = q.w * q.x;
  let qwy = q.w * q.y;
  let qwz = q.w * q.z;

  var normal: vec3<f32> = vec3<f32>(0.);

  normal[0] = 2.0 * (qxz + qwy);
  normal[1] = 2.0 * (qyz - qwx);
  normal[2] = 1.0 - 2.0 * (qxx + qyy);

  return normalize(normal);
}

[[block]] // Deprecated - remove
struct CameraParamsUbo {
  mat_view: mat4x4<f32>;
  mat_proj: mat4x4<f32>;
};

[[block]]
struct SkinMatricesUbo {
  data: array<mat4x4<f32>, 80>;
};

[[group(0), binding(0)]] var<uniform> cameraParams: CameraParamsUbo;
[[group(3), binding(0)]] var<uniform> skinMatrices: SkinMatricesUbo;

[[stage(vertex)]]
fn main(vertex: VertexInput) -> VertexOutput {
  var out: VertexOutput;

  let mat_world = mat4x4<f32>(
    vertex.mat_world_0,
    vertex.mat_world_1,
    vertex.mat_world_2,
    vertex.mat_world_3);

  let skin_transform: mat4x4<f32> =
    vertex.bone_weights.x * skinMatrices.data[vertex.bone_indices.x] +
    vertex.bone_weights.y * skinMatrices.data[vertex.bone_indices.y] +
    vertex.bone_weights.z * skinMatrices.data[vertex.bone_indices.z] +
    vertex.bone_weights.w * skinMatrices.data[vertex.bone_indices.w];
  
  out.world_pos = (mat_world * skin_transform * vec4<f32>(vertex.position, 1.)).xyz;
  out.world_normal = (mat_world * skin_transform *
    vec4<f32>(normal_from_quat(vertex.tbn_quat), 0.)).xyz;
  out.frag_coord = cameraParams.mat_proj * cameraParams.mat_view * vec4<f32>(out.world_pos, 1.);

  return out;
}