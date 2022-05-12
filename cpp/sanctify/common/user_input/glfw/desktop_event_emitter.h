#ifndef SANCTIFY_COMMON_USER_INPUT_GLFW_DESKTOP_EVENT_EMITTER_H
#define SANCTIFY_COMMON_USER_INPUT_GLFW_DESKTOP_EVENT_EMITTER_H

#include <glfw/glfw3.h>

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

 private:
  GLFWwindow* window_;
  entt::registry* world_;
};

}  // namespace sanctify::io

#endif
