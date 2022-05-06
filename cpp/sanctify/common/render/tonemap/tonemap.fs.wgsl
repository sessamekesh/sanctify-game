struct FragmentInput {
  @location(0) uv: vec2<f32>
}

struct FragmentOutput {
  @location(0) ldr_out: vec4<f32>
}

struct TonemappingArguments {
  // NOTE: In a more realistic pipeline, this would be set by a compute pass on
  //  the previous frame. For our purposes, it is fine to adjust as a parameter
  //  at the discretion of the artists in crafting our scene.
  avgLuminosity: f32
}

@group(0) @binding(0) var<uniform> tonemappingArguments: TonemappingArguments;
@group(0) @binding(1) var hdrSampler: sampler;
@group(1) @binding(0) var hdrTexture: texture_2d<f32>;

fn rgb_to_xyY(rgb_color: vec3<f32>) -> vec3<f32> {
  // First convert RGB color space to XYZ color space, using the matrix provided
  // here: http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
  let xyz_color = vec3<f32>(
    dot(vec3<f32>(0.4124564, 0.3575761, 0.1804375), rgb_color),
    dot(vec3<f32>(0.2126729, 0.7151522, 0.0721750), rgb_color),
    dot(vec3<f32>(0.0193339, 0.1191920, 0.9503041), rgb_color)
  );

  // Next, convert from XYZ color space to xyY color space, also using a nice
  // provided here: http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_xyY.html
  // Thanks brucelindbloom <3
  let inv = 1. / (dot(xyz_color, vec3<f32>(1.)) + 0.00001); // prevent divide by zero
  return vec3<f32>(xyz_color.x * inv, xyz_color.y * inv, xyz_color.y);
}

fn xyY_to_rgb(xyY_color: vec3<f32>) -> vec3<f32> {
  // Extract xyY to XYZ color space first. Once again, Bruce Lindbloom shows up in
  // all the reference material I could find, and who am I to go against what has
  // worked so well? http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
  let xyz_color = vec3<f32>(
    (xyY_color.x * xyY_color.z) / xyY_color.y,
    xyY_color.z,
    (1. - xyY_color.x - xyY_color.y) * xyY_color.z / xyY_color.y
  );

  // And finally back into RGB color space by applying the inverse of the matrix
  // used in the rgb_to_xyY implementation (also provided on the same page)
  // http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_xyY.html
  return vec3<f32>(
    dot(vec3<f32>(3.2404542, -1.5371385, -0.4985314), xyz_color),
    dot(vec3<f32>(-0.9692660, 1.8760108, 0.0415560), xyz_color),
    dot(vec3<f32>(0.0556434, -0.2040259, 1.0572252), xyz_color)
  );
}

// ACES filmic tone mapping curve - there are definitely others, but this one is great.
// Adapted from https://www.shadertoy.com/view/WdjSW3, which was in turn adapted from
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
fn tonemap_aces(x: f32) -> f32 {
  let a = 2.51;
  let b = 0.03;
  let c = 2.43;
  let d = 0.59;
  let e = 0.14;
  
  return (x * (a * x + b)) / (x * ( c * x + d) + e);
}

@stage(fragment) fn main(frag: FragmentInput) -> FragmentOutput {
  let hdr_color = textureSample(hdrTexture, hdrSampler, frag.uv).rgb;

  // https://bruop.github.io/tonemapping/
  let hdr_color_xyY = rgb_to_xyY(hdr_color);

  // Select a tonemapping function as appropriate
  let exposure_adjusted_luminance = tonemap_aces(
      hdr_color_xyY.z / (9.6 * tonemappingArguments.avgLuminosity + 0.0001));

  let ldr_rgb_color = xyY_to_rgb(vec3<f32>(hdr_color_xyY.xy, exposure_adjusted_luminance));

  let gamma_adjusted = pow(ldr_rgb_color, vec3<f32>(1. / 2.2));

  return FragmentOutput(vec4<f32>(gamma_adjusted, 1.));
}
