#include <igcore/log.h>

#include "offline_client_app.h"

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