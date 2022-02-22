#ifndef TOOLS_MAP_EDITOR_VIEWS_NAVMESH_PARAMS_VIEW_H
#define TOOLS_MAP_EDITOR_VIEWS_NAVMESH_PARAMS_VIEW_H

#include <util/assimp_loader.h>
#include <util/recast_builder.h>
#include <util/recast_params.h>

#include <filesystem>

namespace mapeditor {

class NavMeshParamsView {
 public:
  NavMeshParamsView(
      std::shared_ptr<RecastParams> recast_params,
      std::shared_ptr<RecastBuilder> recast_builder,
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list);
  ~NavMeshParamsView();

  void render();

 private:
  std::vector<std::string> render_assimp_geo_def(
      int i,
      indigo::igpackgen::pb::AssembleRecastNavMeshAction_AssimpGeoDef& geo_def);

  void sync_from_recast();

 private:
  std::shared_ptr<RecastParams> recast_params_;
  std::shared_ptr<RecastBuilder> recast_builder_;
  std::shared_ptr<AssimpLoader> assimp_loader_;

  std::vector<indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp>
      recast_ops_;

  char asset_root_[256];
  bool is_valid_path_;

  char* full_proto_text_;
  bool text_matches_;
  bool has_text_error_;

  std::shared_ptr<indigo::core::TaskList> async_task_list_;
  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;

  float cell_size_;
  float cell_height_;
  float max_slope_degrees_;
  float walkable_climb_;
  float walkable_height_;
  float agent_radius_;
  float min_region_area_;
  float merge_region_area_;
  float max_contour_error_;
  float max_edge_length_;
  float detail_sample_distance_;
  float detail_max_error_;

  bool is_loading_;
};

}  // namespace mapeditor

#endif
