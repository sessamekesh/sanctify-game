#ifndef TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_PARAMS_H
#define TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_PARAMS_H

#include <igcore/maybe.h>
#include <igpack-gen/proto/igpack-plan.pb.h>

#include <glm/glm.hpp>

namespace mapeditor {

class RecastParams {
 public:
  RecastParams();

  float cell_size() const;
  void cell_size(float value);

  glm::vec3 min_bb() const;
  glm::vec3 max_bb() const;

 private:
  indigo::igpackgen::pb::AssembleRecastNavMeshAction
      recast_nav_mesh_action_proto_;
};

}  // namespace mapeditor

#endif
