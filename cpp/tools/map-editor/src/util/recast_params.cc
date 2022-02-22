#include <google/protobuf/text_format.h>
#include <ignav/recast_compiler.h>
#include <util/recast_params.h>

using namespace indigo;
using namespace mapeditor;

namespace {
const glm::vec3 kDefaultMinBb = glm::vec3(-50.f, -1.f, -50.f);
const glm::vec3 kDefaultMaxBb = glm::vec3(50.f, 20.f, 50.f);
}  // namespace

RecastParams::RecastParams() {}

float RecastParams::cell_size() const {
  if (recast_nav_mesh_action_proto_.cell_size() != 0.f) {
    return recast_nav_mesh_action_proto_.cell_size();
  }

  return nav::kDefaultCellSize;
}

void RecastParams::cell_size(float value) {
  if (value == nav::kDefaultCellSize) {
    recast_nav_mesh_action_proto_.clear_cell_size();
    return;
  }
  recast_nav_mesh_action_proto_.set_cell_size(value);
}

float RecastParams::cell_height() const {
  if (recast_nav_mesh_action_proto_.cell_height() != 0.f) {
    return recast_nav_mesh_action_proto_.cell_height();
  }

  return nav::kDefaultCellHeight;
}

void RecastParams::cell_height(float value) {
  if (value == nav::kDefaultCellHeight) {
    recast_nav_mesh_action_proto_.clear_cell_height();
    return;
  }
  recast_nav_mesh_action_proto_.set_cell_height(value);
}

float RecastParams::max_slope_degrees() const {
  if (recast_nav_mesh_action_proto_.max_slope_degrees() != 0.f) {
    return recast_nav_mesh_action_proto_.max_slope_degrees();
  }

  return nav::kDefaultMaxSlopeDegrees;
}

void RecastParams::max_slope_degrees(float value) {
  if (value == nav::kDefaultMaxSlopeDegrees) {
    recast_nav_mesh_action_proto_.clear_max_slope_degrees();
    return;
  }
  recast_nav_mesh_action_proto_.set_max_slope_degrees(value);
}

float RecastParams::walkable_climb() const {
  if (recast_nav_mesh_action_proto_.walkable_climb() != 0.f) {
    return recast_nav_mesh_action_proto_.walkable_climb();
  }

  return nav::kDefaultWalkableClimb;
}

void RecastParams::walkable_climb(float value) {
  if (value == nav::kDefaultWalkableClimb) {
    recast_nav_mesh_action_proto_.clear_walkable_climb();
    return;
  }
  recast_nav_mesh_action_proto_.set_walkable_climb(value);
}

float RecastParams::walkable_height() const {
  if (recast_nav_mesh_action_proto_.walkable_height() != 0.f) {
    return recast_nav_mesh_action_proto_.walkable_height();
  }

  return nav::kDefaultWalkableHeight;
}

void RecastParams::walkable_height(float value) {
  if (value == nav::kDefaultWalkableHeight) {
    recast_nav_mesh_action_proto_.clear_walkable_height();
    return;
  }
  recast_nav_mesh_action_proto_.set_walkable_height(value);
}

float RecastParams::agent_radius() const {
  if (recast_nav_mesh_action_proto_.agent_radius() == 0.f) {
    return nav::kDefaultAgentRadius;
  }
  return recast_nav_mesh_action_proto_.agent_radius();
}

void RecastParams::agent_radius(float value) {
  if (value == nav::kDefaultAgentRadius) {
    recast_nav_mesh_action_proto_.clear_agent_radius();
    return;
  }
  recast_nav_mesh_action_proto_.set_agent_radius(value);
}

float RecastParams::min_region_area() const {
  if (recast_nav_mesh_action_proto_.min_region_area() == 0.f) {
    return nav::kDefaultMinRegionArea;
  }
  return recast_nav_mesh_action_proto_.min_region_area();
}

void RecastParams::min_region_area(float value) {
  if (value == nav::kDefaultMinRegionArea) {
    recast_nav_mesh_action_proto_.clear_min_region_area();
    return;
  }
  recast_nav_mesh_action_proto_.set_min_region_area(value);
}

float RecastParams::merge_region_area() const {
  if (recast_nav_mesh_action_proto_.merge_region_area() == 0.f) {
    return nav::kDefaultMergeRegionArea;
  }
  return recast_nav_mesh_action_proto_.merge_region_area();
}

void RecastParams::merge_region_area(float value) {
  if (value == nav::kDefaultMergeRegionArea) {
    recast_nav_mesh_action_proto_.clear_merge_region_area();
    return;
  }
  recast_nav_mesh_action_proto_.set_merge_region_area(value);
}

float RecastParams::max_contour_error() const {
  if (recast_nav_mesh_action_proto_.max_contour_error() == 0.f) {
    return nav::kDefaultMaxContourError;
  }
  return recast_nav_mesh_action_proto_.max_contour_error();
}

void RecastParams::max_contour_error(float value) {
  if (value == nav::kDefaultMaxContourError) {
    recast_nav_mesh_action_proto_.clear_max_contour_error();
    return;
  }
  recast_nav_mesh_action_proto_.set_max_contour_error(value);
}

float RecastParams::max_edge_length() const {
  if (recast_nav_mesh_action_proto_.max_edge_length() == 0.f) {
    return nav::kDefaultMaxEdgeLength;
  }
  return recast_nav_mesh_action_proto_.max_edge_length();
}

void RecastParams::max_edge_length(float value) {
  if (value == nav::kDefaultMaxEdgeLength) {
    recast_nav_mesh_action_proto_.clear_max_edge_length();
    return;
  }
  recast_nav_mesh_action_proto_.set_max_edge_length(value);
}

float RecastParams::detail_sample_distance() const {
  if (recast_nav_mesh_action_proto_.detail_sample_distance() == 0.f) {
    return nav::kDefaultDetailSampleDistance;
  }
  return recast_nav_mesh_action_proto_.detail_sample_distance();
}

void RecastParams::detail_sample_distance(float value) {
  if (value == nav::kDefaultDetailSampleDistance) {
    recast_nav_mesh_action_proto_.clear_detail_sample_distance();
    return;
  }
  recast_nav_mesh_action_proto_.set_detail_sample_distance(value);
}

float RecastParams::detail_max_error() const {
  if (recast_nav_mesh_action_proto_.detail_max_error() == 0.f) {
    return nav::kDefaultDetailMaxError;
  }
  return recast_nav_mesh_action_proto_.detail_max_error();
}

void RecastParams::detail_max_error(float value) {
  if (value == nav::kDefaultDetailMaxError) {
    recast_nav_mesh_action_proto_.clear_detail_max_error();
    return;
  }
  recast_nav_mesh_action_proto_.set_detail_max_error(value);
}

glm::vec3 RecastParams::min_bb() const {
  if (recast_nav_mesh_action_proto_.has_bb_override()) {
    return glm::vec3{recast_nav_mesh_action_proto_.bb_override().min_x(),
                     recast_nav_mesh_action_proto_.bb_override().min_y(),
                     recast_nav_mesh_action_proto_.bb_override().min_z()};
  }

  return ::kDefaultMinBb;
}

void RecastParams::bb(glm::vec3 min, glm::vec3 max) {
  if (min == ::kDefaultMinBb && max == ::kDefaultMaxBb) {
    recast_nav_mesh_action_proto_.clear_bb_override();
    return;
  }
  auto* bb = recast_nav_mesh_action_proto_.mutable_bb_override();
  bb->set_min_x(min.x);
  bb->set_min_y(min.y);
  bb->set_min_z(min.z);
  bb->set_max_x(max.x);
  bb->set_max_y(max.y);
  bb->set_max_z(max.z);
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
