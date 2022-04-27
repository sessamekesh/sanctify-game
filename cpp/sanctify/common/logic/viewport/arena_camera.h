#ifndef SANCTIFY_COMMON_LOGIC_VIEWPORT_ARENA_CAMERA_H
#define SANCTIFY_COMMON_LOGIC_VIEWPORT_ARENA_CAMERA_H

#include <glm/glm.hpp>

namespace sanctify::logic {

class ArenaCamera {
  //
  // Construction
  //
 public:
  // Naive construction - pass in all parameters
  ArenaCamera(const glm::vec3& look_at, float tilt_angle, float spin_angle,
              float radius);

  //
  // Stored properties (get/set)
  //
 public:
  // Center point of the screen - i.e., what position is the camera
  // looking at? This will be somewhere on the surface of the arena.
  void set_look_at(const glm::vec3& v);
  const glm::vec3& look_at() const;

  // Vertical tilt of the camera, in radians? An angle of zero would have the
  // camera looking straight down, up to a rotation of positive PI/2 radians
  // which has the camera looking parallel to the ground.
  void set_tilt_angle(float v);
  const float& tilt_angle() const;

  // Horizontal spin of the camera, in radians? An angle of zero places the
  // camera tilting along the -Z axis ("backwards") with a positive increase
  // rotating the camera counter-clockwise towards the +X axis
  void set_spin_angle(float v);
  const float& spin_angle() const;

  // Distance from the inspection point on the ground to the camera
  void set_radius(float v);
  const float& radius() const;

  //
  // Computed properties
  //
 public:
  // World transformation matrix
  glm::mat4 mat_view() const;

  // Where is the camera in 3D space?
  glm::vec3 position() const;

  // What is the screen right vector?
  glm::vec3 screen_right() const;

  // What is the screen up vector?
  glm::vec3 screen_up() const;

 private:
  void normalize_spin();
  void normalize_tilt();

 private:
  glm::vec3 look_at_;
  float tilt_angle_;
  float spin_angle_;
  float radius_;
};

}  // namespace sanctify::logic

#endif
