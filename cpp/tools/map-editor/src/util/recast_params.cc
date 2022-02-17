#include <google/protobuf/text_format.h>
#include <util/recast_params.h>

using namespace indigo;
using namespace mapeditor;

namespace {
const float kDefaultCellSize = 0.25f;
const glm::vec3 kDefaultMinBb = glm::vec3(-50.f, -1.f, -50.f);
const glm::vec3 kDefaultMaxBb = glm::vec3(50.f, 20.f, 50.f);
}  // namespace

RecastParams::RecastParams() {}

float RecastParams::cell_size() const {
  if (recast_nav_mesh_action_proto_.cell_size() != 0.f) {
    return recast_nav_mesh_action_proto_.cell_size();
  }

  return ::kDefaultCellSize;
}

void RecastParams::cell_size(float value) {
  recast_nav_mesh_action_proto_.set_cell_size(value);
}

glm::vec3 RecastParams::min_bb() const {
  if (recast_nav_mesh_action_proto_.has_bb_override()) {
    return glm::vec3{recast_nav_mesh_action_proto_.bb_override().min_x(),
                     recast_nav_mesh_action_proto_.bb_override().min_y(),
                     recast_nav_mesh_action_proto_.bb_override().min_z()};
  }

  return ::kDefaultMinBb;
}

glm::vec3 RecastParams::max_bb() const {
  if (recast_nav_mesh_action_proto_.has_bb_override()) {
    return glm::vec3{recast_nav_mesh_action_proto_.bb_override().max_x(),
                     recast_nav_mesh_action_proto_.bb_override().max_y(),
                     recast_nav_mesh_action_proto_.bb_override().max_z()};
  }

  return ::kDefaultMaxBb;
}

std::vector<igpackgen::pb::AssembleRecastNavMeshAction_RecastOp>
RecastParams::recast_ops() const {
  std::vector<igpackgen::pb::AssembleRecastNavMeshAction_RecastOp> ops;

  for (const auto& op : recast_nav_mesh_action_proto_.recast_build_ops()) {
    ops.push_back(op);
  }

  return ops;
}

void RecastParams::recast_ops(
    std::vector<indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp>
        ops) {
  recast_nav_mesh_action_proto_.clear_recast_build_ops();
  for (const auto& op : ops) {
    *recast_nav_mesh_action_proto_.add_recast_build_ops() = op;
  }
}

std::filesystem::path RecastParams::asset_root() const { return asset_root_; }

void RecastParams::asset_root(std::filesystem::path path) {
  asset_root_ = path;
}

std::string RecastParams::proto_text() const {
  std::string text;
  google::protobuf::TextFormat::PrintToString(recast_nav_mesh_action_proto_,
                                              &text);
  return text;
}

bool RecastParams::proto_text(std::string text) {
  return google::protobuf::TextFormat::ParseFromString(
      text, &recast_nav_mesh_action_proto_);
}
