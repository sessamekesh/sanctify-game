struct LightingParams {
  light_direction: vec3<f32>,
  ambient_coefficient: f32,
  light_color: vec3<f32>,
  specular_power: f32
}

struct ModelFsParams {
  color: vec4<f32>
}

struct CameraParams {
  camera_pos: vec3<f32>
}

struct FragmentIn {
  @location(0) world_pos: vec3<f32>,
  @location(1) world_normal: vec3<f32>
}

@group(0) @binding(2) var<uniform> lightingParams: LightingParams;
@group(0) @binding(3) var<uniform> modelFsParams: ModelFsParams;
@group(0) @binding(4) var<uniform> cameraParams: CameraParams;

@stage(fragment)
fn main(in: FragmentIn) -> @location(0) vec4<f32> {
  let mat_color = modelFsParams.color.xyz;

  let ambient_coefficient = lightingParams.ambient_coefficient;
  let ambient_color = ambient_coefficient * mat_color;

  let light_dir = normalize(-lightingParams.light_direction);
  let normal = normalize(in.world_normal);
  let diffuse_coefficient = (1. - ambient_coefficient) * max(dot(light_dir, normal), 0.);
  let diffuse_color = diffuse_coefficient * mat_color;

  let view_dir = normalize(cameraParams.camera_pos - in.world_pos);
  let halfway_dir = normalize(light_dir + view_dir);
  let specular_coefficient = pow(max(dot(normal, halfway_dir), 0.), lightingParams.specular_power);
  let specular_color = vec3<f32>(0.3) * specular_coefficient;

  return vec4<f32>(ambient_color + diffuse_color + specular_color, 1.);
}
