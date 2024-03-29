#ifndef SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_NET_GAME_SNAPSHOT_H
#define SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_NET_GAME_SNAPSHOT_H

#include <igcore/maybe.h>
#include <igcore/vector.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/gameplay/player_definition_components.h>

#include <entt/entt.hpp>
#include <map>
#include <set>
#include <variant>

namespace sanctify {

class GameSnapshotDiff {
 public:
  enum class ComponentType {
    // Should never be used! Indicates that there was a parsing problem usually
    Invalid,

    MapLocation,
    StandardNavigationParams,
    NavWaypointList,
    BasicPlayerComponent,
    Orientation,
  };

 public:
  GameSnapshotDiff();

  // Construction
  void upsert(uint32_t net_sync_id,
              indigo::core::Maybe<component::MapLocation> map_location);
  void upsert(
      uint32_t net_sync_id,
      indigo::core::Maybe<component::StandardNavigationParams> nav_params);
  void upsert(
      uint32_t net_sync_id,
      indigo::core::Maybe<component::NavWaypointList> nav_waypoint_list);
  void upsert(uint32_t net_sync_id,
              indigo::core::Maybe<component::BasicPlayerComponent>
                  basic_player_component);
  void upsert(uint32_t net_sync_id,
              indigo::core::Maybe<component::OrientationComponent> orientation);

  void delete_entity(uint32_t net_sync_id);
  void delete_component(uint32_t net_sync_id, ComponentType component_type);

  void dest_snapshot_id(uint32_t id) { dest_snapshot_id_ = id; }
  void base_snapshot_id(uint32_t id) { base_snapshot_id_ = id; }
  void snapshot_time(float time);

  // Queries
  uint32_t dest_snapshot_id() const { return dest_snapshot_id_; }
  uint32_t base_snapshot_id() const { return base_snapshot_id_; }
  float snapshot_time() const;

  indigo::core::PodVector<uint32_t> deleted_entities() const;
  indigo::core::PodVector<uint32_t> upserted_entities() const;

  indigo::core::PodVector<ComponentType> deleted_components(
      uint32_t net_sync_id) const;

  indigo::core::Maybe<component::MapLocation> map_location(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<component::NavWaypointList> nav_waypoint_list(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<component::StandardNavigationParams>
  standard_navigation_params(uint32_t net_sync_id) const;
  indigo::core::Maybe<component::BasicPlayerComponent> basic_player_component(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<component::OrientationComponent> orientation(
      uint32_t net_sync_id) const;

  // Serialization
  pb::GameSnapshotDiff serialize() const;
  static GameSnapshotDiff Deserialize(const pb::GameSnapshotDiff& diff);

 private:
  float snapshot_time_;
  uint32_t base_snapshot_id_;
  uint32_t dest_snapshot_id_;

  std::set<uint32_t> upsert_entities_;
  std::set<uint32_t> deleted_entities_;

  std::unordered_map<uint32_t, component::MapLocation> map_location_upserts_;
  std::unordered_map<uint32_t, component::StandardNavigationParams>
      nav_params_upserts_;
  std::unordered_map<uint32_t, component::NavWaypointList>
      nav_waypoint_list_upserts_;
  std::unordered_map<uint32_t, component::BasicPlayerComponent>
      basic_player_component_upserts_;
  std::unordered_map<uint32_t, component::OrientationComponent>
      orientation_upserts_;

  std::unordered_map<uint32_t, indigo::core::PodVector<ComponentType>>
      component_deletes_;
};

class GameSnapshot {
 public:
  GameSnapshot();

  GameSnapshot(const GameSnapshot& o) = default;
  GameSnapshot& operator=(const GameSnapshot& o) = default;

  // Diff generation / application
  static GameSnapshotDiff CreateDiff(const GameSnapshot& base_snapshot,
                                     const GameSnapshot& dest_snapshot);

  static GameSnapshot ApplyDiff(const GameSnapshot& base_snapshot,
                                const GameSnapshotDiff& diff);

  // Construction
  void add(uint32_t net_sync_id,
           indigo::core::Maybe<component::MapLocation> map_location);
  void add(uint32_t net_sync_id,
           indigo::core::Maybe<component::StandardNavigationParams> nav_params);
  void add(uint32_t net_sync_id,
           indigo::core::Maybe<component::NavWaypointList> nav_waypoint_list);
  void add(uint32_t net_sync_id,
           indigo::core::Maybe<component::BasicPlayerComponent>
               basic_player_component);
  void add(uint32_t net_sync_id,
           indigo::core::Maybe<component::OrientationComponent> orientation);
  void snapshot_time(float time);
  void snapshot_id(uint32_t id);
  void delete_entity(uint32_t net_sync_id);
  void delete_component(uint32_t net_sync_id,
                        GameSnapshotDiff::ComponentType component_type);

  // Queries
  indigo::core::Maybe<component::MapLocation> map_location(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<component::NavWaypointList> nav_waypoint_list(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<component::StandardNavigationParams>
  standard_navigation_params(uint32_t net_sync_id) const;
  indigo::core::Maybe<component::BasicPlayerComponent> basic_player_component(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<component::OrientationComponent> orientation(
      uint32_t net_sync_id) const;

  float snapshot_time() const;
  uint32_t snapshot_id() const;
  const std::set<uint32_t>& alive_entities() const;

  // Serialization
  pb::GameSnapshotFull serialize() const;
  static GameSnapshot Deserialize(const pb::GameSnapshotFull& diff);

 private:
  float snapshot_time_;
  uint32_t snapshot_id_;

  std::set<uint32_t> alive_entities_;
  std::unordered_map<uint32_t, component::MapLocation> map_location_components_;
  std::unordered_map<uint32_t, component::StandardNavigationParams>
      standard_nav_params_components_;
  std::unordered_map<uint32_t, component::NavWaypointList>
      nav_waypoint_list_components_;
  std::unordered_map<uint32_t, component::BasicPlayerComponent>
      basic_player_components_;
  std::unordered_map<uint32_t, component::OrientationComponent>
      orientation_components_;
};

}  // namespace sanctify

#endif
