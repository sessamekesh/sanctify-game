#include <igcore/either.h>
#include <sanctify-game-common/net/game_snapshot.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

////////////////////////////////////////////////////
//
// HELPERS
//
////////////////////////////////////////////////////

namespace {
const char* kSnapshotLabel = "GameSnapshot";
const char* kDiffLabel = "GameSnapshotDiff";

pb::GameEntityUpdateMask::ComponentType to_proto(
    GameSnapshotDiff::ComponentType logical) {
  switch (logical) {
    case GameSnapshotDiff::ComponentType::MapLocation:
      return pb::GameEntityUpdateMask::ComponentType::
          GameEntityUpdateMask_ComponentType_MAP_LOCATION;
    case GameSnapshotDiff::ComponentType::NavWaypointList:
      return pb::GameEntityUpdateMask::ComponentType::
          GameEntityUpdateMask_ComponentType_NAV_WAYPOINTS;
    case GameSnapshotDiff::ComponentType::StandardNavigationParams:
      return pb::GameEntityUpdateMask::ComponentType::
          GameEntityUpdateMask_ComponentType_STANDARD_NAVIGATION_PARAMS;
    case GameSnapshotDiff::ComponentType::BasicPlayerComponent:
      return pb::GameEntityUpdateMask::ComponentType::
          GameEntityUpdateMask_ComponentType_BASIC_PLAYER_COMPONENT;
    case GameSnapshotDiff::ComponentType::Orientation:
      return pb::GameEntityUpdateMask::ComponentType::
          GameEntityUpdateMask_ComponentType_ORIENTATION;

    default:
      return pb::GameEntityUpdateMask::ComponentType::
          GameEntityUpdateMask_ComponentType_INVALID;
  }
}

GameSnapshotDiff::ComponentType from_proto(
    pb::GameEntityUpdateMask::ComponentType proto) {
  switch (proto) {
    case pb::GameEntityUpdateMask::ComponentType::
        GameEntityUpdateMask_ComponentType_MAP_LOCATION:
      return GameSnapshotDiff::ComponentType::MapLocation;
    case pb::GameEntityUpdateMask::ComponentType::
        GameEntityUpdateMask_ComponentType_NAV_WAYPOINTS:
      return GameSnapshotDiff::ComponentType::NavWaypointList;
    case pb::GameEntityUpdateMask::ComponentType::
        GameEntityUpdateMask_ComponentType_STANDARD_NAVIGATION_PARAMS:
      return GameSnapshotDiff::ComponentType::StandardNavigationParams;
    case pb::GameEntityUpdateMask::ComponentType::
        GameEntityUpdateMask_ComponentType_BASIC_PLAYER_COMPONENT:
      return GameSnapshotDiff::ComponentType::BasicPlayerComponent;
    case pb::GameEntityUpdateMask::ComponentType::
        GameEntityUpdateMask_ComponentType_ORIENTATION:
      return GameSnapshotDiff::ComponentType::Orientation;

    default:
      return GameSnapshotDiff::ComponentType::Invalid;
  }
}

template <typename T>
Maybe<T> extract(uint32_t net_sync_id,
                 const std::unordered_map<uint32_t, T> map) {
  auto it = map.find(net_sync_id);
  if (it == map.end()) {
    return empty_maybe{};
  }
  return it->second;
}

void set_vec2(pb::Vec2* mut_pb_vec, const glm::vec2& data) {
  mut_pb_vec->set_x(data.x);
  mut_pb_vec->set_y(data.y);
}

glm::vec2 extract_vec2(const pb::Vec2& pb) { return glm::vec2{pb.x(), pb.y()}; }

//
// PB insertion
//
void maybe_add_to_proto(pb::ComponentData* mut_component_data,
                        Maybe<component::MapLocation> map_location) {
  if (map_location.is_empty() || !mut_component_data) {
    return;
  }

  pb::Vec2* location_vec =
      mut_component_data->mutable_map_location()->mutable_location();
  ::set_vec2(location_vec, map_location.get().XZ);
}

void maybe_add_to_proto(pb::ComponentData* mut_component_data,
                        Maybe<component::StandardNavigationParams> nav_params) {
  if (nav_params.is_empty() || !mut_component_data) {
    return;
  }

  mut_component_data->mutable_standard_navigation_params()->set_movement_speed(
      nav_params.get().MovementSpeed);
}

void maybe_add_to_proto(pb::ComponentData* mut_component_data,
                        Maybe<component::NavWaypointList> waypoints) {
  if (waypoints.is_empty() || !mut_component_data) {
    return;
  }

  pb::NavWaypointList* mut_waypoints =
      mut_component_data->mutable_nav_waypoints();

  for (int i = 0; i < waypoints.get().Targets.size(); i++) {
    const auto& target = waypoints.get().Targets[i];
    ::set_vec2(mut_waypoints->add_nav_waypoints(), target);
  }
}

void maybe_add_to_proto(pb::ComponentData* mut_component_data,
                        Maybe<component::BasicPlayerComponent> component) {
  if (component.is_empty() || !mut_component_data) {
    return;
  }

  pb::BasicPlayerComponent* mut_player =
      mut_component_data->mutable_basic_player_component();
}

void maybe_add_to_proto(pb::ComponentData* mut_component_data,
                        Maybe<component::OrientationComponent> orientation) {
  if (orientation.is_empty() || !mut_component_data) {
    return;
  }

  pb::Orientation* pb = mut_component_data->mutable_orientation();
  pb->set_orientation(orientation.get().orientation);
}

//
// PB extraction
//
Maybe<component::MapLocation> extract_map_location(
    const pb::ComponentData& component_data) {
  if (!component_data.has_map_location() ||
      !component_data.map_location().has_location()) {
    return empty_maybe{};
  }

  return component::MapLocation{
      ::extract_vec2(component_data.map_location().location())};
}

Maybe<component::NavWaypointList> extract_waypoint_list(
    const pb::ComponentData& component_data) {
  if (!component_data.has_nav_waypoints()) {
    return empty_maybe{};
  }

  size_t num_waypoints = component_data.nav_waypoints().nav_waypoints_size();
  PodVector<glm::vec2> waypoints(num_waypoints);
  for (int i = 0; i < num_waypoints; i++) {
    waypoints.push_back(
        ::extract_vec2(component_data.nav_waypoints().nav_waypoints(i)));
  }

  return component::NavWaypointList{std::move(waypoints)};
}

Maybe<component::StandardNavigationParams> extract_nav_params(
    const pb::ComponentData& component_data) {
  if (!component_data.has_standard_navigation_params()) {
    return empty_maybe{};
  }

  return component::StandardNavigationParams{
      component_data.standard_navigation_params().movement_speed()};
}

Maybe<component::BasicPlayerComponent> extract_basic_player_component(
    const pb::ComponentData& component_data) {
  if (!component_data.has_basic_player_component()) {
    return empty_maybe{};
  }

  return component::BasicPlayerComponent{};
}

Maybe<component::OrientationComponent> extract_orientation(
    const pb::ComponentData& data) {
  if (!data.has_orientation()) {
    return empty_maybe{};
  }

  return component::OrientationComponent{data.orientation().orientation()};
}

//
// Diff generation
//
Maybe<Either<component::MapLocation, GameSnapshotDiff::ComponentType>>
generate_map_location_diff(uint32_t net_sync_id, const GameSnapshot& base,
                           const GameSnapshot& dest) {
  using EitherType =
      Either<component::MapLocation, GameSnapshotDiff::ComponentType>;

  Maybe<component::MapLocation> base_value = base.map_location(net_sync_id);
  Maybe<component::MapLocation> dest_value = dest.map_location(net_sync_id);

  if (base_value.is_empty() && dest_value.is_empty()) {
    return empty_maybe{};
  }

  if (base_value.has_value() && dest_value.is_empty()) {
    return EitherType(right(GameSnapshotDiff::ComponentType::MapLocation));
  }

  // Dest value must be present, but we don't care if base value is present or
  // not by this code path

  if (base_value.get() == dest_value.get()) {
    return empty_maybe{};
  }

  return EitherType(left(dest_value.move()));
}

void diff_map_location(uint32_t net_sync_id, const GameSnapshot& base,
                       const GameSnapshot& dest, GameSnapshotDiff& mut_diff) {
  auto maybe_map_diff = ::generate_map_location_diff(net_sync_id, base, dest);
  if (maybe_map_diff.has_value()) {
    auto map_diff = maybe_map_diff.move();
    if (map_diff.is_left()) {
      mut_diff.upsert(net_sync_id, map_diff.get_left());
    } else {
      mut_diff.delete_component(net_sync_id, map_diff.get_right());
    }
  }
}

Maybe<Either<component::NavWaypointList, GameSnapshotDiff::ComponentType>>
generate_waypoint_diff(uint32_t net_sync_id, const GameSnapshot& base,
                       const GameSnapshot& dest) {
  using EitherType =
      Either<component::NavWaypointList, GameSnapshotDiff::ComponentType>;

  auto base_value = base.nav_waypoint_list(net_sync_id);
  auto dest_value = dest.nav_waypoint_list(net_sync_id);

  if (base_value.is_empty() && dest_value.is_empty()) {
    return empty_maybe{};
  }

  if (base_value.has_value() && dest_value.is_empty()) {
    return EitherType(right(GameSnapshotDiff::ComponentType::NavWaypointList));
  }

  if (base_value.get() == dest_value.get()) {
    return empty_maybe{};
  }

  return EitherType(left(dest_value.move()));
}

void diff_waypoints(uint32_t net_sync_id, const GameSnapshot& base,
                    const GameSnapshot& dest, GameSnapshotDiff& mut_diff) {
  auto maybe_waypoint_diff = ::generate_waypoint_diff(net_sync_id, base, dest);
  if (maybe_waypoint_diff.has_value()) {
    auto waypoint_diff = maybe_waypoint_diff.move();
    if (waypoint_diff.is_left()) {
      mut_diff.upsert(net_sync_id, waypoint_diff.get_left());
    } else {
      mut_diff.delete_component(net_sync_id, waypoint_diff.get_right());
    }
  }
}

Maybe<Either<component::StandardNavigationParams,
             GameSnapshotDiff::ComponentType>>
generate_nav_params_diff(uint32_t net_sync_id, const GameSnapshot& base,
                         const GameSnapshot& dest) {
  using EitherType = Either<component::StandardNavigationParams,
                            GameSnapshotDiff::ComponentType>;

  auto base_value = base.standard_navigation_params(net_sync_id);
  auto dest_value = dest.standard_navigation_params(net_sync_id);

  if (base_value.is_empty() && dest_value.is_empty()) {
    return empty_maybe{};
  }

  if (base_value.has_value() && dest_value.is_empty()) {
    return EitherType(
        right(GameSnapshotDiff::ComponentType::StandardNavigationParams));
  }

  if (base_value.get() == dest_value.get()) {
    return empty_maybe{};
  }

  return EitherType(left(dest_value.move()));
}

void diff_nav_params(uint32_t net_sync_id, const GameSnapshot& base,
                     const GameSnapshot& dest, GameSnapshotDiff& mut_diff) {
  auto maybe_nav_params_diff =
      ::generate_nav_params_diff(net_sync_id, base, dest);
  if (maybe_nav_params_diff.has_value()) {
    auto nav_params_diff = maybe_nav_params_diff.move();
    if (nav_params_diff.is_left()) {
      mut_diff.upsert(net_sync_id, nav_params_diff.get_left());
    } else {
      mut_diff.delete_component(net_sync_id, nav_params_diff.get_right());
    }
  }
}

Maybe<Either<component::BasicPlayerComponent, GameSnapshotDiff::ComponentType>>
generate_basic_player_diff(uint32_t net_sync_id, const GameSnapshot& base,
                           const GameSnapshot& dest) {
  using EitherType =
      Either<component::BasicPlayerComponent, GameSnapshotDiff::ComponentType>;

  auto base_value = base.basic_player_component(net_sync_id);
  auto dest_value = dest.basic_player_component(net_sync_id);

  if (base_value.is_empty() && dest_value.is_empty()) {
    return empty_maybe{};
  }

  if (base_value.has_value() && dest_value.is_empty()) {
    return EitherType(
        right(GameSnapshotDiff::ComponentType::BasicPlayerComponent));
  }

  if (base_value.get() == dest_value.get()) {
    return empty_maybe{};
  }

  return EitherType(left(dest_value.move()));
}

void diff_basic_player_components(uint32_t net_sync_id,
                                  const GameSnapshot& base,
                                  const GameSnapshot& dest,
                                  GameSnapshotDiff& mut_diff) {
  auto maybe_basic_params_diff =
      ::generate_basic_player_diff(net_sync_id, base, dest);
  if (maybe_basic_params_diff.has_value()) {
    auto basic_params_diff = maybe_basic_params_diff.move();
    if (basic_params_diff.is_left()) {
      mut_diff.upsert(net_sync_id, basic_params_diff.left_move());
    } else {
      mut_diff.delete_component(net_sync_id, basic_params_diff.get_right());
    }
  }
}

Maybe<Either<component::OrientationComponent, GameSnapshotDiff::ComponentType>>
generate_orientation_diff(uint32_t net_sync_id, const GameSnapshot& base,
                          const GameSnapshot& dest) {
  using EitherType =
      Either<component::OrientationComponent, GameSnapshotDiff::ComponentType>;

  auto base_value = base.orientation(net_sync_id);
  auto dest_value = dest.orientation(net_sync_id);

  if (base_value.is_empty() && dest_value.is_empty()) {
    return empty_maybe{};
  }

  if (base_value.has_value() && dest_value.is_empty()) {
    return EitherType(right(GameSnapshotDiff::ComponentType::Orientation));
  }

  if (base_value.get() == dest_value.get()) {
    return empty_maybe{};
  }

  return EitherType(left(dest_value.move()));
}

void diff_orientation(uint32_t net_sync_id, const GameSnapshot& base,
                      const GameSnapshot& dest, GameSnapshotDiff& mut_diff) {
  auto maybe_orientation_diff =
      ::generate_orientation_diff(net_sync_id, base, dest);
  if (maybe_orientation_diff.has_value()) {
    auto orientation_diff = maybe_orientation_diff.move();
    if (orientation_diff.is_left()) {
      mut_diff.upsert(net_sync_id, orientation_diff.left_move());
    } else {
      mut_diff.delete_component(net_sync_id, orientation_diff.get_right());
    }
  }
}

}  // namespace

////////////////////////////////////////////////////
//
// DIFF IMPL
//
////////////////////////////////////////////////////

GameSnapshotDiff::GameSnapshotDiff()
    : base_snapshot_id_(0u), dest_snapshot_id_(0u), snapshot_time_(0.f) {}

void GameSnapshotDiff::upsert(uint32_t net_sync_id,
                              Maybe<component::MapLocation> map_location) {
  if (map_location.is_empty()) {
    return;
  }

  upsert_entities_.insert(net_sync_id);
  map_location_upserts_.emplace(net_sync_id, map_location.move());
}

void GameSnapshotDiff::upsert(
    uint32_t net_sync_id,
    Maybe<component::StandardNavigationParams> nav_params) {
  if (nav_params.is_empty()) {
    return;
  }
  upsert_entities_.insert(net_sync_id);
  nav_params_upserts_.emplace(net_sync_id, nav_params.move());
}

void GameSnapshotDiff::upsert(
    uint32_t net_sync_id,
    Maybe<component::NavWaypointList> nav_waypoints_list) {
  if (nav_waypoints_list.is_empty()) {
    return;
  }

  upsert_entities_.insert(net_sync_id);
  nav_waypoint_list_upserts_.emplace(net_sync_id, nav_waypoints_list.move());
}

void GameSnapshotDiff::upsert(
    uint32_t net_sync_id,
    Maybe<component::BasicPlayerComponent> basic_player_component) {
  if (basic_player_component.is_empty()) {
    return;
  }

  upsert_entities_.insert(net_sync_id);
  basic_player_component_upserts_.emplace(net_sync_id,
                                          basic_player_component.move());
}

void GameSnapshotDiff::upsert(
    uint32_t net_sync_id, Maybe<component::OrientationComponent> orientation) {
  if (orientation.is_empty()) {
    return;
  }

  upsert_entities_.insert(net_sync_id);
  orientation_upserts_.emplace(net_sync_id, orientation.move());
}

void GameSnapshotDiff::delete_entity(uint32_t net_sync_id) {
  deleted_entities_.insert(net_sync_id);
}

void GameSnapshotDiff::delete_component(
    uint32_t net_sync_id, GameSnapshotDiff::ComponentType component_type) {
  upsert_entities_.insert(net_sync_id);

  auto it = component_deletes_.find(net_sync_id);
  if (it == component_deletes_.end()) {
    core::PodVector<ComponentType> v(1);
    v.push_back(component_type);
    component_deletes_.emplace(net_sync_id, std::move(v));
  } else {
    it->second.push_back(component_type);
  }
}

void GameSnapshotDiff::snapshot_time(float time) { snapshot_time_ = time; }

float GameSnapshotDiff::snapshot_time() const { return snapshot_time_; }

PodVector<uint32_t> GameSnapshotDiff::deleted_entities() const {
  PodVector<uint32_t> rsl(deleted_entities_.size());

  for (auto net_sync_id : deleted_entities_) {
    rsl.push_back(net_sync_id);
  }

  rsl.in_place_sort();
  return rsl;
}

PodVector<uint32_t> GameSnapshotDiff::upserted_entities() const {
  PodVector<uint32_t> rsl(upsert_entities_.size());

  for (auto net_sync_id : upsert_entities_) {
    rsl.push_back(net_sync_id);
  }

  rsl.in_place_sort();
  return rsl;
}

Maybe<component::MapLocation> GameSnapshotDiff::map_location(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, map_location_upserts_);
}

Maybe<component::NavWaypointList> GameSnapshotDiff::nav_waypoint_list(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, nav_waypoint_list_upserts_);
}

Maybe<component::StandardNavigationParams>
GameSnapshotDiff::standard_navigation_params(uint32_t net_sync_id) const {
  return ::extract(net_sync_id, nav_params_upserts_);
}

Maybe<component::BasicPlayerComponent> GameSnapshotDiff::basic_player_component(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, basic_player_component_upserts_);
}

Maybe<component::OrientationComponent> GameSnapshotDiff::orientation(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, orientation_upserts_);
}

PodVector<GameSnapshotDiff::ComponentType> GameSnapshotDiff::deleted_components(
    uint32_t net_sync_id) const {
  auto it = component_deletes_.find(net_sync_id);
  if (it == component_deletes_.end()) {
    return PodVector<ComponentType>(1);
  }

  return it->second;
}

pb::GameSnapshotDiff GameSnapshotDiff::serialize() const {
  pb::GameSnapshotDiff diff_proto{};
  diff_proto.set_game_time(snapshot_time_);
  diff_proto.set_base_snapshot_id(base_snapshot_id());
  diff_proto.set_dest_snapshot_id(dest_snapshot_id());

  // Upserted entities
  for (auto upsert_net_sync_id : upsert_entities_) {
    pb::GameEntityUpdateMask* entity_upsert_proto =
        diff_proto.add_upsert_entities();

    // Net sync ID
    entity_upsert_proto->set_net_sync_id(upsert_net_sync_id);

    // Upsert components
    pb::ComponentData* mut_component_data =
        entity_upsert_proto->mutable_upsert_components();

    ::maybe_add_to_proto(mut_component_data, map_location(upsert_net_sync_id));
    ::maybe_add_to_proto(mut_component_data,
                         standard_navigation_params(upsert_net_sync_id));
    ::maybe_add_to_proto(mut_component_data,
                         nav_waypoint_list(upsert_net_sync_id));
    ::maybe_add_to_proto(mut_component_data,
                         basic_player_component(upsert_net_sync_id));
    ::maybe_add_to_proto(mut_component_data, orientation(upsert_net_sync_id));

    // Delete dropped components
    PodVector<ComponentType> delete_components =
        deleted_components(upsert_net_sync_id);
    for (int i = 0; i < delete_components.size(); i++) {
      entity_upsert_proto->add_remove_components(
          ::to_proto(delete_components[i]));
    }
  }

  // Deleted entities
  PodVector<uint32_t> delete_entities = deleted_entities();
  for (int i = 0; i < delete_entities.size(); i++) {
    diff_proto.add_remove_entities(delete_entities[i]);
  }

  // Finished!
  return diff_proto;
}

GameSnapshotDiff GameSnapshotDiff::Deserialize(
    const pb::GameSnapshotDiff& diff) {
  GameSnapshotDiff diff_model{};
  diff_model.snapshot_time_ = diff.game_time();
  diff_model.dest_snapshot_id_ = diff.dest_snapshot_id();
  diff_model.base_snapshot_id_ = diff.base_snapshot_id();

  // Upserted entities
  for (const pb::GameEntityUpdateMask& upserted_entities :
       diff.upsert_entities()) {
    uint32_t net_sync_id = upserted_entities.net_sync_id();
    const pb::ComponentData& upserted_components =
        upserted_entities.upsert_components();

    // Upserted components
    diff_model.upsert(net_sync_id, ::extract_map_location(upserted_components));
    diff_model.upsert(net_sync_id, ::extract_nav_params(upserted_components));
    diff_model.upsert(net_sync_id,
                      ::extract_waypoint_list(upserted_components));
    diff_model.upsert(net_sync_id,
                      ::extract_basic_player_component(upserted_components));
    diff_model.upsert(net_sync_id, ::extract_orientation(upserted_components));

    // Deleted components
    for (int i = 0; i < upserted_entities.remove_components_size(); i++) {
      diff_model.delete_component(
          net_sync_id, ::from_proto(upserted_entities.remove_components(i)));
    }
  }

  // Removed entities
  for (uint32_t removed_entity : diff.remove_entities()) {
    diff_model.delete_entity(removed_entity);
  }

  return diff_model;
}

////////////////////////////////////////////////////
//
// SNAPSHOT IMPL
//
////////////////////////////////////////////////////

GameSnapshot::GameSnapshot() : snapshot_id_(0u), snapshot_time_(0.f) {}

GameSnapshotDiff GameSnapshot::CreateDiff(const GameSnapshot& base_snapshot,
                                          const GameSnapshot& dest_snapshot) {
  GameSnapshotDiff diff{};
  diff.snapshot_time(dest_snapshot.snapshot_time_);
  diff.base_snapshot_id(base_snapshot.snapshot_id());
  diff.dest_snapshot_id(dest_snapshot.snapshot_id());

  // Upsert all elements found in the base snapshot, unless they are identical
  // in the destination snapshot. This upsertion may include deleting components
  for (uint32_t dest_net_sync_id : dest_snapshot.alive_entities_) {
    ::diff_map_location(dest_net_sync_id, base_snapshot, dest_snapshot, diff);
    ::diff_nav_params(dest_net_sync_id, base_snapshot, dest_snapshot, diff);
    ::diff_waypoints(dest_net_sync_id, base_snapshot, dest_snapshot, diff);
    ::diff_basic_player_components(dest_net_sync_id, base_snapshot,
                                   dest_snapshot, diff);
    ::diff_orientation(dest_net_sync_id, base_snapshot, dest_snapshot, diff);
  }

  // Find all the entities in the base snapshot that do not exist in the
  // destination, and delete them.
  for (uint32_t base_net_sync_id : base_snapshot.alive_entities_) {
    if (dest_snapshot.alive_entities_.find(base_net_sync_id) ==
        dest_snapshot.alive_entities_.end()) {
      diff.delete_entity(base_net_sync_id);
    }
  }

  return diff;
}

GameSnapshot GameSnapshot::ApplyDiff(const GameSnapshot& base_snapshot,
                                     const GameSnapshotDiff& diff) {
  if (base_snapshot.snapshot_id() != diff.base_snapshot_id()) {
    Logger::err(kSnapshotLabel)
        << "Attempting to apply diff for base " << diff.base_snapshot_id()
        << " to base " << base_snapshot.snapshot_id()
        << " - the result will likely be nonsense!";
  }

  GameSnapshot dest = base_snapshot;
  dest.snapshot_time(diff.snapshot_time());
  dest.snapshot_id(diff.dest_snapshot_id());

  PodVector<uint32_t> deleted_entities = diff.deleted_entities();
  for (int i = 0; i < deleted_entities.size(); i++) {
    dest.delete_entity(deleted_entities[i]);
  }

  PodVector<uint32_t> upserted_entities = diff.upserted_entities();
  for (int i = 0; i < upserted_entities.size(); i++) {
    uint32_t net_sync_id = upserted_entities[i];

    PodVector<GameSnapshotDiff::ComponentType> deleted_component_types =
        diff.deleted_components(net_sync_id);
    for (int j = 0; j < deleted_component_types.size(); j++) {
      GameSnapshotDiff::ComponentType deleted_component_type =
          deleted_component_types[j];
      dest.delete_component(net_sync_id, deleted_component_type);
    }

    dest.add(net_sync_id, diff.map_location(net_sync_id));
    dest.add(net_sync_id, diff.standard_navigation_params(net_sync_id));
    dest.add(net_sync_id, diff.nav_waypoint_list(net_sync_id));
    dest.add(net_sync_id, diff.basic_player_component(net_sync_id));
    dest.add(net_sync_id, diff.orientation(net_sync_id));
  }

  return dest;
}

void GameSnapshot::add(uint32_t net_sync_id,
                       Maybe<component::MapLocation> map_location) {
  if (map_location.is_empty()) {
    return;
  }

  alive_entities_.insert(net_sync_id);
  map_location_components_[net_sync_id] = map_location.move();
}

void GameSnapshot::add(uint32_t net_sync_id,
                       Maybe<component::StandardNavigationParams> nav_params) {
  if (nav_params.is_empty()) {
    return;
  }

  alive_entities_.insert(net_sync_id);
  standard_nav_params_components_[net_sync_id] = nav_params.move();
}

void GameSnapshot::add(uint32_t net_sync_id,
                       Maybe<component::NavWaypointList> nav_waypoints) {
  if (nav_waypoints.is_empty()) {
    return;
  }

  alive_entities_.insert(net_sync_id);
  nav_waypoint_list_components_[net_sync_id] = nav_waypoints.move();
}

void GameSnapshot::add(uint32_t net_sync_id,
                       Maybe<component::BasicPlayerComponent> component) {
  if (component.is_empty()) {
    return;
  }

  alive_entities_.insert(net_sync_id);
  basic_player_components_[net_sync_id] = component.move();
}

void GameSnapshot::add(uint32_t net_sync_id,
                       Maybe<component::OrientationComponent> component) {
  if (component.is_empty()) {
    return;
  }

  alive_entities_.insert(net_sync_id);
  orientation_components_[net_sync_id] = component.move();
}

void GameSnapshot::snapshot_time(float time) { snapshot_time_ = time; }

void GameSnapshot::snapshot_id(uint32_t id) { snapshot_id_ = id; }

void GameSnapshot::delete_entity(uint32_t net_sync_id) {
  alive_entities_.erase(net_sync_id);
  map_location_components_.erase(net_sync_id);
  standard_nav_params_components_.erase(net_sync_id);
  nav_waypoint_list_components_.erase(net_sync_id);
  basic_player_components_.erase(net_sync_id);
  orientation_components_.erase(net_sync_id);
}

void GameSnapshot::delete_component(uint32_t net_sync_id,
                                    GameSnapshotDiff::ComponentType type) {
  switch (type) {
    case GameSnapshotDiff::ComponentType::MapLocation:
      map_location_components_.erase(net_sync_id);
      return;
    case GameSnapshotDiff::ComponentType::NavWaypointList:
      nav_waypoint_list_components_.erase(net_sync_id);
      return;
    case GameSnapshotDiff::ComponentType::StandardNavigationParams:
      standard_nav_params_components_.erase(net_sync_id);
      return;
    case GameSnapshotDiff::ComponentType::BasicPlayerComponent:
      basic_player_components_.erase(net_sync_id);
      return;
    case GameSnapshotDiff::ComponentType::Orientation:
      orientation_components_.erase(net_sync_id);
      return;
  }
}

Maybe<component::MapLocation> GameSnapshot::map_location(
    uint32_t net_sync_id) const {
  if (alive_entities_.find(net_sync_id) == alive_entities_.end()) {
    return empty_maybe{};
  }

  return ::extract(net_sync_id, map_location_components_);
}

Maybe<component::NavWaypointList> GameSnapshot::nav_waypoint_list(
    uint32_t net_sync_id) const {
  if (alive_entities_.find(net_sync_id) == alive_entities_.end()) {
    return empty_maybe{};
  }

  return ::extract(net_sync_id, nav_waypoint_list_components_);
}

Maybe<component::StandardNavigationParams>
GameSnapshot::standard_navigation_params(uint32_t net_sync_id) const {
  if (alive_entities_.find(net_sync_id) == alive_entities_.end()) {
    return empty_maybe{};
  }

  return ::extract(net_sync_id, standard_nav_params_components_);
}

Maybe<component::BasicPlayerComponent> GameSnapshot::basic_player_component(
    uint32_t net_sync_id) const {
  if (alive_entities_.find(net_sync_id) == alive_entities_.end()) {
    return empty_maybe{};
  }

  return ::extract(net_sync_id, basic_player_components_);
}

Maybe<component::OrientationComponent> GameSnapshot::orientation(
    uint32_t net_sync_id) const {
  if (alive_entities_.find(net_sync_id) == alive_entities_.end()) {
    return empty_maybe{};
  }

  return ::extract(net_sync_id, orientation_components_);
}

float GameSnapshot::snapshot_time() const { return snapshot_time_; }

uint32_t GameSnapshot::snapshot_id() const { return snapshot_id_; }

pb::GameSnapshotFull GameSnapshot::serialize() const {
  pb::GameSnapshotFull proto{};
  proto.set_game_time(snapshot_time());
  proto.set_snapshot_id(snapshot_id());

  for (uint32_t net_sync_id : alive_entities_) {
    pb::GameEntity* game_entity = proto.add_game_entities();
    game_entity->set_net_sync_id(net_sync_id);

    pb::ComponentData* mut_component_data =
        game_entity->mutable_component_data();
    ::maybe_add_to_proto(mut_component_data, map_location(net_sync_id));
    ::maybe_add_to_proto(mut_component_data, nav_waypoint_list(net_sync_id));
    ::maybe_add_to_proto(mut_component_data,
                         standard_navigation_params(net_sync_id));
    ::maybe_add_to_proto(mut_component_data,
                         basic_player_component(net_sync_id));
    ::maybe_add_to_proto(mut_component_data, orientation(net_sync_id));
  }

  return proto;
}

GameSnapshot GameSnapshot::Deserialize(const pb::GameSnapshotFull& diff) {
  GameSnapshot snapshot{};
  snapshot.snapshot_time(diff.game_time());
  snapshot.snapshot_id(diff.snapshot_id());

  for (const pb::GameEntity& proto_entity : diff.game_entities()) {
    uint32_t net_sync_id = proto_entity.net_sync_id();
    const pb::ComponentData& component_data = proto_entity.component_data();

    snapshot.add(net_sync_id, ::extract_map_location(component_data));
    snapshot.add(net_sync_id, ::extract_nav_params(component_data));
    snapshot.add(net_sync_id, ::extract_waypoint_list(component_data));
    snapshot.add(net_sync_id, ::extract_basic_player_component(component_data));
    snapshot.add(net_sync_id, ::extract_orientation(component_data));
  }

  return snapshot;
}

const std::set<uint32_t>& GameSnapshot::alive_entities() const {
  return alive_entities_;
}
