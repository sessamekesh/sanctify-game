#ifndef TOOLS_MAP_EDITOR_MAP_EDITOR_APP_H
#define TOOLS_MAP_EDITOR_MAP_EDITOR_APP_H

#include <app_window.h>
#include <igasync/executor_thread.h>
#include <igasync/frame_task_scheduler.h>
#include <igasync/task_list.h>
#include <igcore/either.h>
#include <util/recast_builder.h>
#include <util/recast_params.h>
#include <views/navmesh_params_view.h>
#include <views/viewport_view.h>

namespace mapeditor {

class MapEditorApp : public std::enable_shared_from_this<MapEditorApp> {
 public:
  static std::shared_ptr<MapEditorApp> Create();
  ~MapEditorApp();

  void update(float dt);
  void render();
  bool should_quit();
  void execute_tasks_until(
      std::chrono::high_resolution_clock::time_point end_time);

 private:
  void setup_imgui();
  void teardown_imgui();

  void on_resize_swap_chain();
  void render_gui();

  void render_preview_settings_view();

 private:
  MapEditorApp(
      std::shared_ptr<MapEditorAppWindow> base,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      indigo::core::Vector<std::shared_ptr<indigo::core::ExecutorThread>>
          executor_threads);

  std::shared_ptr<MapEditorAppWindow> base_;
  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;
  std::shared_ptr<indigo::core::TaskList> async_task_list_;

  uint32_t next_swap_chain_width_, next_swap_chain_height_;
  float time_to_next_swap_chain_update_;

  indigo::core::Vector<std::shared_ptr<indigo::core::ExecutorThread>>
      executor_threads_;

  // Shared logical resources
  std::shared_ptr<RecastParams> recast_params_;
  std::shared_ptr<RecastBuilder> recast_builder_;

  // Views
  std::shared_ptr<ViewportView> viewport_view_;
  std::shared_ptr<NavMeshParamsView> nav_mesh_params_view_;
};

}  // namespace mapeditor

#endif
