#ifndef TOOLS_MAP_EDITOR_VIEWS_NAVMESH_PARAMS_VIEW_H
#define TOOLS_MAP_EDITOR_VIEWS_NAVMESH_PARAMS_VIEW_H

#include <util/assimp_loader.h>
#include <util/recast_params.h>

namespace mapeditor {

class NavMeshParamsView {
 public:
  NavMeshParamsView(
      std::shared_ptr<RecastParams> recast_params,
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list);

  void render();

 private:
  std::shared_ptr<RecastParams> recast_params_;
  std::shared_ptr<AssimpLoader> assimp_loader_;

  std::shared_ptr<indigo::core::TaskList> async_task_list_;
  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;

  float cell_size_;
};

}  // namespace mapeditor

#endif
