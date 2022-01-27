#include <emscripten/bind.h>
#include <sanctify_client_app.h>

using namespace emscripten;
using namespace sanctify;

EMSCRIPTEN_BINDINGS(SanctifyGameClient) {
  class_<SanctifyClientApp>("SanctifyClientApp")
      .class_function("Create", &SanctifyClientApp::Create)
      .smart_ptr<std::shared_ptr<SanctifyClientApp>>("SanctifyClientApp")
      .function("update", &SanctifyClientApp::update)
      .function("render", &SanctifyClientApp::render)
      .function("run_tasks_for", &SanctifyClientApp::run_tasks_for)
      .function("should_quit", &SanctifyClientApp::should_quit);
}