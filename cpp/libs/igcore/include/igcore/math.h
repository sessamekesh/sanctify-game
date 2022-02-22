#ifndef _LIB_IGCORE_MATH_H_
#define _LIB_IGCORE_MATH_H_

#include <glm/glm.hpp>

namespace indigo::core::igmath {

float clamp(float v, float min, float max);
float angle_clamp(float angle);
float lerp(float t, float a, float b);

constexpr glm::vec3 UNIT_X = glm::vec3(1.f, 0.f, 0.f);
constexpr glm::vec3 UNIT_Y = glm::vec3(0.f, 1.f, 0.f);
constexpr glm::vec3 UNIT_Z = glm::vec3(0.f, 0.f, 1.f);

/// <summary>
/// Get the Cartesian unit vector of a lookAt direction, given spherical
///  coordinate system input parameters.
/// </summary>
/// <param name="spin">
///   Angle of spin - at 0 is +Z direction (forward), spinning towards +X axis
/// </param>
/// <param name="tilt">
///   Angle of tilt (Y coordinate) - no tilt at 0, increase towards PI/2 for +Y
/// </param>
/// <returns></returns>
glm::vec3 camera_sphere_coords(float spin, float tilt);

glm::vec4 extract_normal_quat(glm::vec3 tangent, glm::vec3 bitangent,
                              glm::vec3 normal);

glm::mat4 transform_matrix(glm::vec3 pos = {0.f, 0.f, 0.f},
                                  glm::vec3 rot_axis = {0.f, 1.f, 0.f},
                                  float rot_angle = 0.f,
                                  glm::vec3 scl = {1.f, 1.f, 1.f});

}  // namespace indigo::core::igmath

#endif
