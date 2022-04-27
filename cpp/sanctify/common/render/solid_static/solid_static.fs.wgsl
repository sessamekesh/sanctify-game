struct FragmentInput {
  // Geometry lighting attributes
  @location(0) world_pos: vec3<f32>,
  @location(1) normal: vec3<f32>,

  // Material parameters
  @location(2) albedo: vec3<f32>,
  @location(3) metallic: f32,
  @location(4) roughness: f32,
  @location(5) ao: f32
}

struct FragmentOutput {
  @location(0) hdr_color: vec4<f32>
}

struct LightingParams {
  light_direction: vec3<f32>,
  ambient_coefficient: f32,
  light_color: vec3<f32>,
  specular_power: f32
}

struct CameraFragmentParams {
  camera_pos: vec3<f32>
}

@group(0) @binding(1) var<uniform> cameraParams: CameraFragmentParams;
@group(1) @binding(0) var<uniform> lightingParams: LightingParams;

/////////////////////// PBR functions ///////////////////////
// https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.1.lighting/1.1.pbr.fs
let PI = 3.14159265359;
fn DistributionGGX(N: vec3<f32>, H: vec3<f32>, roughness: f32) -> f32 {
    let a = roughness*roughness;
    let a2 = a*a;
    let NdotH = max(dot(N, H), 0.);
    let NdotH2 = NdotH*NdotH;

    let nom   = a2;
    var denom = (NdotH2 * (a2 - 1.) + 1.);
    denom = PI * denom * denom;

    return nom / denom;
}
fn GeometrySchlickGGX(NdotV: f32, roughness: f32) -> f32 {
    let r = (roughness + 1.);
    let k = (r*r) / 8.;

    let nom   = NdotV;
    let denom = NdotV * (1. - k) + k;

    return nom / denom;
}
fn GeometrySmith(N: vec3<f32>, V: vec3<f32>, L: vec3<f32>, roughness: f32) -> f32 {
    let NdotV = max(dot(N, V), 0.0);
    let NdotL = max(dot(N, L), 0.0);
    let ggx2 = GeometrySchlickGGX(NdotV, roughness);
    let ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
fn fresnelSchlick(cosTheta: f32, F0: vec3<f32>) -> vec3<f32> {
    return F0 + (1. - F0) * pow(clamp(1. - cosTheta, 0., 1.), 5.);
}
/////////////////////// PBR functions ///////////////////////


@stage(fragment)
fn main(frag: FragmentInput) -> FragmentOutput {
  let N = normalize(frag.normal);
  let V = normalize(cameraParams.camera_pos - frag.world_pos);

  // Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
  // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
  var F0 = vec3<f32>(0.04);
  F0 = mix(F0, frag.albedo, frag.metallic);

  // Reflectance equation
  var Lo = vec3<f32>(0.);
  {
    let L = normalize(lightingParams.light_direction - frag.world_pos);
    let H = normalize(V + L);
    let radiance = lightingParams.light_color;

    // Cook-Torrance BRDF
    let NDF = DistributionGGX(N, H, frag.roughness);
    let G   = GeometrySmith(N, V, L, frag.roughness);
    let F   = fresnelSchlick(clamp(dot(H, V), 0., 1.), F0);

    let numerator = NDF * G * F;
    let denominator = 4. * max(dot(N, V), 0.) * max(dot(N, L), 0.) + 0.00001; // 0.00001 to prevent divide by zero.
    let specular = numerator / denominator;

    // kS is equal to Fresnel
    let kS = F;

    // For energy conservation, the diffuse and specular light can't be above 1.0 (unless the
    // surface emits light); to preserve this condition, the diffuse component (kD) should
    // equal 1. -= kS
    var kD = max((1. - kS), vec3<f32>(0.));

    // Multiply kD by the inverse metalness such that only non-metals have a diffuse component, or a
    // linear blend if partly metal (pure metals have no diffuse light).
    kD *= 1. - frag.metallic;

    let NdotL = max(dot(N, L), 0.);

    // Add outgoing radiance
    Lo += (kD * frag.albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
  }

  // Ambient lighting
  let ambient = lightingParams.ambient_coefficient * lightingParams.light_color * frag.ao * frag.albedo;

  let color = ambient + Lo;

  return FragmentOutput(vec4<f32>(ambient + Lo, 1.));
}
