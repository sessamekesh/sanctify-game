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

  glm::vec3 min_bb() const;
  glm::vec3 max_bb() const;

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
