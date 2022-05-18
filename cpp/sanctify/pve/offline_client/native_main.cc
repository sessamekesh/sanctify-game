#include <igcore/log.h>

#include "offline_client_app.h"

// GLFW event listeners for native
namespace {
std::shared_ptr<sanctify::pve::OfflineClientApp> gApp = nullptr;

bool gFocused = false;
double gLastX = 0.;
double gLastY = 0.;

void glfw_cursor_position(GLFWwindow* window, double xpos, double ypos) {
  if (gFocused) {
    gApp->mouse_move(sanctify::io::MouseMoveEvent::of(
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS,
        glm::vec2(gLastX, gLastY), glm::vec2(xpos, ypos)));
  }

  gLastX = xpos;
  gLastY = ypos;
}

void glfw_cursor_enter_callback(GLFWwindow* window, int entered) {
  gFocused = entered;
  glfwGetCursorPos(window, &gLastX, &gLastY);

  gApp->focus_change(sanctify::io::FocusChangeEvent::of(gFocused));
}

void attach_glfw_input_listeners(
    std::shared_ptr<sanctify::pve::OfflineClientApp> app) {
  gApp = app;

  auto* window = gApp->get_window();

  glfwSetCursorPosCallback(window, glfw_cursor_position);
  glfwSetCursorEnterCallback(window, glfw_cursor_enter_callback);
}

void detach_glfw_input_listeners() {
  if (!gApp) return;

  auto* window = gApp->get_window();

  glfwSetCursorEnterCallback(window, nullptr);
  glfwSetCursorPosCallback(window, nullptr);
  gApp = nullptr;
}

class raiiEventHandlerThing {
 public:
  raiiEventHandlerThing() = delete;
  raiiEventHandlerThing(std::shared_ptr<sanctify::pve::OfflineClientApp> app)
      : app_(app) {
    attach_glfw_input_listeners(app);
  }
  ~raiiEventHandlerThing() { detach_glfw_input_listeners(); }
  raiiEventHandlerThing(const raiiEventHandlerThing&) = delete;
  raiiEventHandlerThing(raiiEventHandlerThing&&) = default;
  raiiEventHandlerThing& operator=(const raiiEventHandlerThing&) = delete;
  raiiEventHandlerThing& operator=(raiiEventHandlerThing&&) = default;

 private:
  std::shared_ptr<sanctify::pve::OfflineClientApp> app_;
};

}  // namespace

/*************************************************************************\

             FUNCTION MAIN - entry point of the application

\*************************************************************************/
int main() {
  // TODO (sessamekesh): Read this in from a configuration file instead of
  // creating it manually here.
  sanctify::pve::pb::PveOfflineClientConfig config{};
  config.mutable_render_settings()->set_swap_chain_resize_latency(0.5f);
  config.mutable_debug_settings()->set_frame_time(1.f / 60.f);

  auto app =
      sanctify::pve::OfflineClientApp::Create(config.SerializeAsString());

  if (app == nullptr) {
    indigo::core::Logger::log("main")
        << "Failed to create Sanctify offline client app";
    return -1;
  }

  ::raiiEventHandlerThing evts(app);

  using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;

  auto last_time = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(FpSeconds(0.02));

  while (!app->should_quit()) {
    auto now = std::chrono::high_resolution_clock::now();

    auto dt = FpSeconds(now - last_time).count();
    auto end_frame_at = now + std::chrono::milliseconds(12);
    last_time = now;

#ifndef NDEBUG
    if (config.debug_settings().frame_time() > 0.f) {
      dt = config.debug_settings().frame_time();
    }
#endif

    app->update(dt);
    app->render();

    app->run_tasks_for(
        FpSeconds(end_frame_at - std::chrono::high_resolution_clock::now())
            .count());

    glfwPollEvents();
  }

  indigo::core::Logger::log("main") << "Successful natural exit of game";

  return 0;
}