#include <igcore/log.h>
#include <sanctify_client_app.h>

int main() {
  auto app = sanctify::SanctifyClientApp::Create();
  if (app == nullptr) {
    core::Logger::log("main") << "Failed to create Sanctify app";
    return -1;
  }

  auto last_time = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(2));

  using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;

  while (!app->should_quit()) {
    auto now = std::chrono::high_resolution_clock::now();

    auto dt = FpSeconds(now - last_time).count();
    auto end_frame_at = now + std::chrono::milliseconds(12);
    last_time = now;

    app->update(dt);
    app->render();

    app->run_tasks_for(
        FpSeconds(end_frame_at - std::chrono::high_resolution_clock::now())
            .count());

    glfwPollEvents();
  }

  return 0;
}