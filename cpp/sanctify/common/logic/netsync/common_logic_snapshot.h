#ifndef SANCTIFY_COMMON_LOGIC_NETSYNC_COMMON_LOGIC_SNAPSHOT_H
#define SANCTIFY_COMMON_LOGIC_NETSYNC_COMMON_LOGIC_SNAPSHOT_H

#include <common/logic/locomotion/locomotion.h>
#include <common/logic/update_common/tick_time_elapsed.h>
#include <common/pb/common_logic_snapshot.pb.h>
#include <igcore/maybe.h>

#include "common_logic_snapshot_diff.h"

/**
 * Logical representation of a game state, at least from the point of view of
 *  common logical components. Can be serialized as a protocol buffer, sent
 *  over the wire, and deserialized on the other side.
 *
 * Both client and server applications need to be capable of assembling
 *  snapshots from an EnTT world registry.
 *
 * Client applications also need to be capable of applying a diff to their world
 *  state. Methods here may vary and fall out of scope of this object.
 */

namespace sanctify::logic {

class CommonLogicSnapshot {
 public:
  CommonLogicSnapshot();

  CommonLogicSnapshot(const CommonLogicSnapshot&) = default;
  CommonLogicSnapshot& operator=(const CommonLogicSnapshot&) = default;

  // Diff generation / application
  static CommonLogicDiff CreateDiff(const CommonLogicSnapshot& base,
                                    const CommonLogicSnapshot& dest);
  static CommonLogicSnapshot ApplyDiff(const CommonLogicSnapshot& base,
                                       const CommonLogicDiff& diff);

  // Context components
  void set(CtxSimTime sim_time);

  // Components
  void add(uint32_t net_sync_id, MapLocationComponent map_location);
  void add(uint32_t net_sync_id, OrientationComponent orientation);
  void add(uint32_t net_sync_id, NavWaypointListComponent nav_waypoints);
  void add(uint32_t net_sync_id, StandardNavigationParamsComponent nav_params);

  // Context component queries
  indigo::core::Maybe<CtxSimTime> sim_time() const;

  // Component queries
  indigo::core::Maybe<MapLocationComponent> map_location(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<OrientationComponent> orientation(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<NavWaypointListComponent> nav_waypoint_list(
      uint32_t net_sync_id) const;
  indigo::core::Maybe<StandardNavigationParamsComponent>
  standard_navigation_params(uint32_t net_sync_id) const;

  // Serialization
  common::pb::SnapshotFull serialize() const;
  static CommonLogicSnapshot Deserialize(const common::pb::SnapshotFull& pb);

 private:
  std::set<uint32_t> alive_entities_;

  indigo::core::Maybe<CtxSimTime> sim_time_;

  std::unordered_map<uint32_t, MapLocationComponent> map_locations_;
  std::unordered_map<uint32_t, OrientationComponent> orientations_;
  std::unordered_map<uint32_t, NavWaypointListComponent> nav_waypoints_;
  std::unordered_map<uint32_t, StandardNavigationParamsComponent> nav_params_;
};

}  // namespace sanctify::logic

#endif
