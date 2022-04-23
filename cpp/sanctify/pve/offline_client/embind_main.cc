#include <common/simple_client_app/evil_emscripten_hacks.h>
#include <emscripten/bind.h>

#include "offline_client_app.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(SanctifyPveOfflineClient) {
  class_<sanctify::pve::OfflineClientApp>("SanctifyPveOfflineClient")
      .class_function("Create", &sanctify::pve::OfflineClientApp::Create)
      .smart_ptr<std::shared_ptr<sanctify::pve::OfflineClientApp>>(
          "SanctifyPveOfflineClient")
      .function("update", &sanctify::pve::OfflineClientApp::update)
      .function("render", &sanctify::pve::OfflineClientApp::render)
      .function("run_tasks_for",
                &sanctify::pve::OfflineClientApp::run_tasks_for)
      .function("should_quit", &sanctify::pve::OfflineClientApp::should_quit);

  function("evil_hack_set_preferred_wgpu_texture_cb",
           &sanctify::evil_hacks::set_preferred_texture_js_callback);
}
