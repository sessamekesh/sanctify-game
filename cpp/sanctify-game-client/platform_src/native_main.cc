#include <app_base.h>
#include <igcore/log.h>

int main() {
  auto app_create_rsl = AppBase::Create();

  if (app_create_rsl.is_right()) {
    indigo::core::Logger::err("main")
        << "Failed to create app - "
        << ::app_base_create_error_text(app_create_rsl.get_right());
    return -1;
  }

  auto app = app_create_rsl.left_move();

  while (!glfwWindowShouldClose(app->Window)) {
    glfwPollEvents();
  }
}