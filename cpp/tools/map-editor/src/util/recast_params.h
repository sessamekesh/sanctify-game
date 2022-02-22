#ifndef TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_PARAMS_H
#define TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_PARAMS_H

#include <igcore/maybe.h>
#include <igpack-gen/proto/igpack-plan.pb.h>

#include <filesystem>
#include <glm/glm.hpp>

namespace mapeditor {

class RecastParams {
 public:
  RecastParams();

  float cell_size() const;
  void cell_size(float value);

  float cell_height() const;
  void cell_height(float value);

  float max_slope_degrees() const;
  void max_slope_degrees(float value);

  float walkable_climb() const;
  void walkable_climb(float value);

  float walkable_height() const;
  void walkable_height(float value);

  float agent_radius() const;
  void agent_radius(float value);

  float min_region_area() const;
  void min_region_area(float value);

  float merge_region_area() const;
  void merge_region_area(float value);

  float max_contour_error() const;
  void max_contour_error(float value);

  float max_edge_length() const;
  void max_edge_length(float value);

  float detail_sample_distance() const;
  void detail_sample_distance(float value);

  float detail_max_error() const;
  void detail_max_error(float value);

  glm::vec3 min_bb() const;
  glm::vec3 max_bb() const;
  void bb(glm::vec3 min, glm::vec3 max);

  std::vector<indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp>
  recast_ops() const;

  void recast_ops(
      std::vector<indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp>
          ops);

  std::filesystem::path asset_root() const;
  void asset_root(std::filesystem::path path);

  std::string proto_text() const;
  bool proto_text(std::string text);

 private:
  indigo::igpackgen::pb::AssembleRecastNavMeshAction
      recast_nav_mesh_action_proto_;

  std::filesystem::path asset_root_;
};

}  // namespace mapeditor

#endif
