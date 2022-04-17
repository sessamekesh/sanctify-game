#include "proto_serialize.h"

using namespace sanctify;
using namespace logic;

// Vec2
void logic::serialize(common::pb::Vec2* mut_pb, glm::vec2 value) {
  mut_pb->set_x(value.x);
  mut_pb->set_y(value.y);
}

glm::vec2 logic::deserialize(const common::pb::Vec2& pb) {
  return {pb.x(), pb.y()};
}

// CtxSimTime
void logic::serialize(common::pb::CtxSimTimeComponent* mut_pb,
                      CtxSimTime time) {
  mut_pb->set_sim_time_seconds(time.simTimeSeconds);
}

CtxSimTime logic::deserialize(const common::pb::CtxSimTimeComponent& pb) {
  return {pb.sim_time_seconds()};
}

// MapLocationComponent
void logic::serialize(common::pb::MapLocationComponent* mut_cb,
                      MapLocationComponent val) {
  ::serialize(mut_cb->mutable_position(), val.position);
}

MapLocationComponent logic::deserialize(
    const common::pb::MapLocationComponent& pb) {
  return {::deserialize(pb.position())};
}

// OrientationComponent
void logic::serialize(common::pb::OrientationComponent* mut_cb,
                      OrientationComponent val) {
  mut_cb->set_orientation(val.orientation);
}

OrientationComponent logic::deserialize(
    const common::pb::OrientationComponent& pb) {
  return {pb.orientation()};
}

// NavWaypointListComponent
void logic::serialize(common::pb::NavWaypointListComponent* mut_cb,
                      const NavWaypointListComponent& val) {
  for (int i = 0; i < val.targets.size(); i++) {
    ::serialize(mut_cb->add_targets(), val.targets[i]);
  }
}

NavWaypointListComponent logic::deserialize(
    const common::pb::NavWaypointListComponent& pb) {
  indigo::core::PodVector<glm::vec2> waypoints(pb.targets_size());
  for (int i = 0; i < pb.targets_size(); i++) {
    waypoints.push_back(::deserialize(pb.targets(i)));
  }
  return {std::move(waypoints)};
}

// StandardNavigationParamsComponent
void logic::serialize(common::pb::StandardNavigationParamsComponent* mut_pb,
                      StandardNavigationParamsComponent val) {
  mut_pb->set_movement_speed(val.movementSpeed);
}

StandardNavigationParamsComponent logic::deserialize(
    const common::pb::StandardNavigationParamsComponent& pb) {
  return {pb.movement_speed()};
}
