#include <igcore/math.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace indigo;
using namespace core;

float igmath::clamp(float v, float min, float max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

float igmath::angle_clamp(float angle) {
  while (angle < 0.f) {
    angle += glm::two_pi<float>();
  }
  while (angle >= 2.f) {
    angle -= glm::two_pi<float>();
  }

  return angle;
}

glm::vec3 igmath::camera_sphere_coords(float spin, float tilt) {
  return glm::vec3(glm::sin(spin) * glm::cos(tilt), glm::sin(tilt),
                   glm::cos(spin) * glm::cos(tilt));
}

float igmath::lerp(float t, float a, float b) { return t * b + (1.f - t) * a; }

glm::vec4 igmath::extract_normal_quat(glm::vec3 tangent, glm::vec3 bitangent,
                                      glm::vec3 normal) {
  glm::mat3 tbn(tangent, bitangent, normal);
  auto rot = glm::quat_cast(tbn);
  if (rot.w < 0.f) rot *= -1.f;

  return glm::vec4(rot.x, rot.y, rot.z, rot.w);
}

glm::mat4 igmath::transform_matrix(glm::vec3 pos, glm::vec3 rot_axis,
                                          float rot_angle, glm::vec3 scl) {
  return glm::translate(glm::mat4(1.f), pos) *
         glm::toMat4(glm::angleAxis(rot_angle, rot_axis)) *
         glm::scale(glm::mat4(1.f), scl);
}
