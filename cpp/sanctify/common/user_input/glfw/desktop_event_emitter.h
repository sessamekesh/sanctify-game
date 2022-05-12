#ifndef SANCTIFY_COMMON_USER_INPUT_GLFW_DESKTOP_EVENT_EMITTER_H
#define SANCTIFY_COMMON_USER_INPUT_GLFW_DESKTOP_EVENT_EMITTER_H

#include <GLFW/glfw3.h>
#include <igecs/world_view.h>

#include <entt/entt.hpp>

namespace sanctify::io {

class GlfwDesktopEventEmitter {
 public:
  GlfwDesktopEventEmitter(entt::registry* world);
  ~GlfwDesktopEventEmitter();
  GlfwDesktopEventEmitter(const GlfwDesktopEventEmitter&) = delete;
  GlfwDesktopEventEmitter& operator=(const GlfwDesktopEventEmitter&) = delete;
  GlfwDesktopEventEmitter(GlfwDesktopEventEmitter&&) = default;
  GlfwDesktopEventEmitter& operator=(GlfwDesktopEventEmitter&&) = default;

  void attach_to_window(GLFWwindow* window);
  void detach();

  entt::registry* world() { return world_; }
  indigo::igecs::WorldView* thin_view() { return &thin_view_; }

 private:
  GLFWwindow* window_;
  entt::registry* world_;
  indigo::igecs::WorldView thin_view_;
};

}  // namespace sanctify::io

#endif
