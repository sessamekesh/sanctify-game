#ifndef SANCTIFY_COMMON_LOGIC_NETSYNC_COMMON_LOGIC_SNAPSHOT_DIFF_H
#define SANCTIFY_COMMON_LOGIC_NETSYNC_COMMON_LOGIC_SNAPSHOT_DIFF_H

#include <common/logic/locomotion/locomotion.h>
#include <common/logic/update_common/tick_time_elapsed.h>
#include <igcore/maybe.h>
#include <sanctify/common/proto/common_logic_snapshot.pb.h>

#include <set>
#include <unordered_map>

/**
 * Logical representation of a diff between two game states, at least as related
 *  to common logic components. Includes proto serialization/deserialization
 *  methods to allow transferring over the wire.
 */

namespace sanctify::logic {

class CommonLogicDiff {
 public:
  CommonLogicDiff();

  //
  // Context Construction
  //
  void upsert(CtxSimTime sim_time);
  void delete_ctx_component(common::proto::CtxComponentType ctx_component_type);

  //
  // Entity Construction
  //
  void upsert(uint32_t net_sync_id, MapLocationComponent map_location);
  void upsert(uint32_t net_sync_id, OrientationComponent orientation);
  void upsert(uint32_t net_sync_id, NavWaypointListComponent nav_waypoint_list);
  void upsert(uint32_t net_sync_id,
              StandardNavigationParamsComponent standard_navigation_params);

  void delete_entity(uint32_t net_sync_id);
  void delete_component(uint32_t net_sync_id,
                        common::proto::ComponentType component_type);

  //
  // Queries
  //
  const std::set<uint32_t>& deleted_entities() const;
  const std::set<uint32_t>& upserted_entities() const;
  std::set<common::proto::ComponentType> deleted_components(
      uint32_t net_sync_id) const;
  std::set<common::proto::CtxComponentType> deleted_ctx_components() const;

  indigo::core::Maybe<CtxSimTime> sim_time() const;

  indigo::core::Maybe<MapLocationComponent> map_location(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<OrientationComponent> orientation(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<NavWaypointListComponent> nav_waypoint_list(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<StandardNavigationParamsComponent>
  standard_navigation_params(uint32_t net_sync_id) const;

  //
  // Serialization
  //
  common::proto::SnapshotDiff serialize() const;
  static CommonLogicDiff Deserialize(const common::proto::SnapshotDiff& diff);

 private:
  // Entity upserts
  std::set<uint32_t> upserted_entities_;
  std::unordered_map<uint32_t, MapLocationComponent> map_location_upserts_;
  std::unordered_map<uint32_t, OrientationComponent> orientation_upserts_;
  std::unordered_map<uint32_t, NavWaypointListComponent>
      nav_waypoint_list_upserts_;
  std::unordered_map<uint32_t, StandardNavigationParamsComponent>
      standard_navigation_params_upserts_;

  // Entity deletes
  std::unordered_map<uint32_t, std::set<common::proto::ComponentType>>
      deleted_components_;
  std::set<uint32_t> deleted_entities_;

  // Context upserts / deletes
  indigo::core::Maybe<CtxSimTime> sim_time_;
  std::set<common::proto::CtxComponentType> deleted_ctx_components_;
};

}  // namespace sanctify::logic

#endif
