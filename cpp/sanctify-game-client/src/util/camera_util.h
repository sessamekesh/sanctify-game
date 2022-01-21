#ifndef SANCTIFY_GAME_CLIENT_SRC_UTIL_CAMERA_UTIL_H
#define SANCTIFY_GAME_CLIENT_SRC_UTIL_CAMERA_UTIL_H

#include <igcore/maybe.h>
#include <io/arena_camera_controller/arena_camera_input.h>
#include <render/camera/arena_camera.h>

#include <glm/glm.hpp>
namespace sanctify {

namespace camera_util {

/// <summary>
/// Get the direction of a picking ray from where the user clicked on the screen
/// into the scene. Used for broad picking.
/// </summary>
/// <param name="fovy">Field of view along the y direction, in radians</param>
/// <param name="aspect">Aspect ratio of the screen (width / height)</param>
/// <param name="camera_direction">
///   Direction the camera is pointing, in world coordinates
/// </param>
/// <param name="camera_up">Camera up vector, in world coordinates</param>
/// <param name="x_pct">
///   Mouse X position in range [0, 1]
/// </param>
/// <param name="y_pct">
///   Mouse Y position in range [0, 1]
/// </param>
/// <returns>A vector in world space representing the picking ray</returns>
glm::vec3 get_camera_pick_ray(float fovy, float aspect,
                              glm::vec3 camera_direction, glm::vec3 camera_up,
                              float x_pct, float y_pct);

indigo::core::Maybe<glm::vec3> pick_to_xz_plane(float y,
                                                const glm::vec3& origin,
                                                const glm::vec3& pick_ray);

//
// Camera event handling
//
struct CameraEventHandler {
  CameraEventHandler(ArenaCamera* camera_ref, float screen_width,
                     float screen_height, float fovy);

  ArenaCamera* Camera;
  float ScreenWidth;
  float ScreenHeight;
  float Fovy;

  void operator()(const IArenaCameraInput::SnapToPlayer& input);

  void operator()(const IArenaCameraInput::DragCanvasPointToPoint& canvas_drag);
};

}  // namespace camera_util

}  // namespace sanctify

#endif
