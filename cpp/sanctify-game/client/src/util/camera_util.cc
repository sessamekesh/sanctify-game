#include <igcore/log.h>
#include <util/camera_util.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace sanctify;
using Logger = indigo::core::Logger;

glm::vec3 camera_util::get_camera_pick_ray(float fovy, float aspect,
                                           glm::vec3 camera_direction,
                                           glm::vec3 camera_up, float x_pct,
                                           float y_pct) {
  // https://stackoverflow.com/questions/29997209/opengl-c-mouse-ray-picking-glmunproject
  float pick_x = x_pct * 2.f - 1.f;
  float pick_y = y_pct * 2.f - 1.f;

  glm::mat4 proj = glm::perspective(fovy, aspect, 0.1f, 4000.f);
  glm::mat4 view = glm::lookAt(glm::vec3(0.f), camera_direction, camera_up);

  glm::mat4 inv_vp = glm::inverse(proj * view);
  glm::vec4 screen_pos = glm::vec4(pick_x, -pick_y, 1.f, 1.f);
  glm::vec4 world_pos = inv_vp * screen_pos;

  return glm::normalize(glm::vec3(world_pos));
}

indigo::core::Maybe<glm::vec3> camera_util::pick_to_xz_plane(
    float y, const glm::vec3& origin, const glm::vec3& pick_ray) {
  if (pick_ray.y < 0.0001f && pick_ray.y > -0.0001f) {
    return indigo::core::empty_maybe{};
  }

  float t = (y - origin.y) / pick_ray.y;

  return origin + pick_ray * t;
}

camera_util::CameraEventHandler::CameraEventHandler(ArenaCamera* camera_ref,
                                                    float screen_width,
                                                    float screen_height,
                                                    float fovy)
    : Camera(camera_ref),
      ScreenWidth(screen_width),
      ScreenHeight(screen_height),
      Fovy(fovy) {}

void camera_util::CameraEventHandler::operator()(
    const IArenaCameraInput::SnapToPlayer& input) {
  // Not yet implemented
}

void camera_util::CameraEventHandler::operator()(
    const IArenaCameraInput::DragCanvasPointToPoint& canvas_drag) {
  // Raycast from start point to end point to the Y=camera_y plane, calculate
  // the XZ offset along that plane, and move the camera accordingly.
  float start_x = 1.f - canvas_drag.ScreenSpaceStart.x / ScreenWidth;
  float start_y = canvas_drag.ScreenSpaceStart.y / ScreenHeight;
  float end_x = 1.f - canvas_drag.ScreenSpaceFinish.x / ScreenWidth;
  float end_y = canvas_drag.ScreenSpaceFinish.y / ScreenHeight;

  glm::vec3 start_ray = camera_util::get_camera_pick_ray(
      Fovy, ScreenWidth / ScreenHeight, Camera->look_at() - Camera->position(),
      glm::vec3(0.f, 1.f, 0.f), start_x, start_y);
  glm::vec3 end_ray = camera_util::get_camera_pick_ray(
      Fovy, ScreenWidth / ScreenHeight, Camera->look_at() - Camera->position(),
      glm::vec3(0.f, 1.f, 0.f), end_x, end_y);

  auto maybe_start_pos =
      camera_util::pick_to_xz_plane(0.f, Camera->position(), start_ray);
  auto maybe_end_pos =
      camera_util::pick_to_xz_plane(0.f, Camera->position(), end_ray);

  if (maybe_start_pos.is_empty() || maybe_end_pos.is_empty()) {
    Logger::log("CameraEventHandler")
        << "Drag action failed, because either start or end picking ray are "
           "incomplete";
    return;
  }

  float world_dx = maybe_end_pos.get().x - maybe_start_pos.get().x;
  float world_dz = maybe_end_pos.get().z - maybe_start_pos.get().z;

  if (world_dx != 0.f && world_dz != 0.f) {
    Camera->set_look_at(Camera->look_at() + glm::vec3(world_dx, 0.f, world_dz));
  }
}