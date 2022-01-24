struct FragmentInput {
  [[location(0)]] world_pos: vec3<f32>;
  [[location(1)]] world_normal: vec3<f32>;
};

struct FragmentOutput {
  [[location(0)]] ldr_out_color: vec4<f32>;
};

[[block]]
struct LightingParams {
  light_direction: vec3<f32>;
  ambient_coefficient: f32;
  light_color: vec3<f32>;
  specular_power: f32;
};

[[block]]
struct SolidColorParams {
  object_color: vec3<f32>;
};

[[block]]
struct CameraFragmentParams {
  camera_pos: vec3<f32>;
};

[[group(0), binding(1)]] var<uniform> cameraParams: CameraFragmentParams;
[[group(1), binding(0)]] var<uniform> colorParams: SolidColorParams;
[[group(2), binding(0)]] var<uniform> lightingParams: LightingParams;

[[stage(fragment)]]
fn main(frag: FragmentInput) -> FragmentOutput {
  let mat_color = colorParams.object_color;

  let ambient_coefficient = lightingParams.ambient_coefficient;
  let ambient_color = ambient_coefficient * mat_color;

  let light_dir = normalize(-lightingParams.light_direction);
  let normal = normalize(frag.world_normal);
  let diffuse_coefficient = (1. - ambient_coefficient) * max(dot(light_dir, normal), 0.);
  let diffuse_color = diffuse_coefficient * mat_color;

  let view_dir = normalize(cameraParams.camera_pos - frag.world_pos);
  let halfway_dir = normalize(light_dir + view_dir);
  let specular_coefficient = pow(max(dot(normal, halfway_dir), 0.), lightingParams.specular_power);
  let specular_color = vec3<f32>(0.3) * specular_coefficient;

  var out: FragmentOutput;
  out.ldr_out_color = vec4<f32>(ambient_color + diffuse_color + specular_color, 1.);
  return out;
}