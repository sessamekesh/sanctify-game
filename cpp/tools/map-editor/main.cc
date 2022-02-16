#include <igcore/log.h>
#include <map_editor_app.h>

#include <iostream>

const char* kLogLabel = "MapEditor::main";

using namespace indigo;
using namespace core;

int main(int argc, char** argv) {
  Logger::log(kLogLabel) << "Starting up map editor app...";
  auto app = mapeditor::MapEditorApp::Create();
  if (app == nullptr) {
    Logger::err(kLogLabel) << "Failed to create MapEditorApp";
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
    app->execute_tasks_until(end_frame_at);

    glfwPollEvents();
  }

  return 0;
}