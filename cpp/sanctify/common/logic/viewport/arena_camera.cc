#include "arena_camera.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace sanctify;
using namespace logic;

namespace {
const glm::vec3 kUp = glm::vec3(0.f, 1.f, 0.f);
}

ArenaCamera::ArenaCamera(const glm::vec3& look_at, float tilt_angle,
                         float spin_angle, float radius)
    : look_at_(look_at),
      tilt_angle_(tilt_angle),
      spin_angle_(spin_angle),
      radius_(radius) {}

void ArenaCamera::set_look_at(const glm::vec3& v) { look_at_ = v; }

const glm::vec3& ArenaCamera::look_at() const { return look_at_; }

void ArenaCamera::set_tilt_angle(float v) {
  tilt_angle_ = v;
  normalize_tilt();
}

const float& ArenaCamera::tilt_angle() const { return tilt_angle_; }

void ArenaCamera::set_spin_angle(float v) {
  spin_angle_ = v;
  normalize_spin();
}

const float& ArenaCamera::spin_angle() const { return spin_angle_; }

void ArenaCamera::set_radius(float v) { radius_ = glm::max(v, 0.001f); }

const float& ArenaCamera::radius() const { return radius_; }

glm::mat4 ArenaCamera::mat_view() const {
  glm::vec3 pos = position();
  return glm::lookAt(pos, look_at_, ::kUp);
}

glm::vec3 ArenaCamera::position() const {
  glm::vec3 unit_radius = {
      glm::sin(tilt_angle_) * glm::sin(spin_angle_),
      glm::cos(tilt_angle_),
      glm::sin(tilt_angle_) * -glm::cos(spin_angle_),
  };

  return look_at_ + unit_radius * radius_;
}

glm::vec3 ArenaCamera::screen_right() const {
  return glm::vec3(glm::cos(spin_angle_), 0.f, glm::sin(spin_angle_));
}

glm::vec3 ArenaCamera::screen_up() const {
  return glm::vec3(-glm::sin(spin_angle_), 0.f, glm::cos(spin_angle_));
}

void ArenaCamera::normalize_spin() {
  spin_angle_ = glm::min(glm::max(glm::two_pi<float>(), spin_angle_), 0.f);
}

void ArenaCamera::normalize_tilt() {
  tilt_angle_ = glm::min(glm::max(glm::half_pi<float>(), tilt_angle_), 0.f);
}
