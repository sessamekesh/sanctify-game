#include "common_logic_snapshot.h"

#include <common/logic/netsync/proto_serialize.h>
#include <igcore/either.h>
#include <igcore/log.h>

using namespace sanctify;
using namespace logic;
using namespace indigo;
using namespace core;

namespace {

const char* kLogLabel = "CommonLogicSnapshot";

//
// Diff generation utils
//
template <typename ComponentT>
Maybe<Either<ComponentT, common::pb::ComponentType>> gen_diff(
    uint32_t net_sync_id, const CommonLogicSnapshot& base,
    const CommonLogicSnapshot& dest,
    Maybe<ComponentT> (CommonLogicSnapshot::*getter)(uint32_t) const,
    common::pb::ComponentType component_type) {
  Maybe<ComponentT> base_value = (base.*getter)(net_sync_id);
  Maybe<ComponentT> dest_value = (dest.*getter)(net_sync_id);

  if (base_value.is_empty() && dest_value.is_empty()) {
    return empty_maybe{};
  }

  if (base_value.has_value() && dest_value.is_empty()) {
    return {right<common::pb::ComponentType>(component_type)};
  }

  if (base_value.get() == dest_value.get()) {
    return empty_maybe{};
  }

  return {left(dest_value.move())};
}

template <typename ComponentT>
void diff_component(uint32_t net_sync_id, const CommonLogicSnapshot& base,
                    const CommonLogicSnapshot& dest, CommonLogicDiff* mut_diff,
                    Maybe<ComponentT> (CommonLogicSnapshot::*getter)(uint32_t)
                        const,
                    common::pb::ComponentType component_type) {
  auto diff_rsl = ::gen_diff(net_sync_id, base, dest, getter, component_type);

  if (diff_rsl.has_value()) {
    auto diff = diff_rsl.move();
    if (diff.is_left()) {
      mut_diff->upsert(net_sync_id, diff.left_move());
    } else {
      mut_diff->delete_component(net_sync_id, diff.get_right());
    }
  }
}

template <typename CtxT>
Maybe<Either<CtxT, common::pb::CtxComponentType>> gen_ctx_diff(
    const CommonLogicSnapshot& base, const CommonLogicSnapshot& dest,
    Maybe<CtxT> (CommonLogicSnapshot::*getter)() const,
    common::pb::CtxComponentType component_type) {
  Maybe<CtxT> base_value = (base.*getter)();
  Maybe<CtxT> dest_value = (dest.*getter)();

  if (base_value.is_empty() && dest_value.is_empty()) {
    return empty_maybe{};
  }

  if (base_value.has_value() && dest_value.is_empty()) {
    return {right<common::pb::CtxComponentType>(component_type)};
  }

  if (base_value.get() == dest_value.get()) {
    return empty_maybe{};
  }

  return {left(dest_value.move())};
}

template <typename CtxT>
void diff_ctx_component(const CommonLogicSnapshot& base,
                        const CommonLogicSnapshot& dest,
                        CommonLogicDiff* mut_diff,
                        Maybe<CtxT> (CommonLogicSnapshot::*getter)() const,
                        common::pb::CtxComponentType component_type) {
  auto diff_rsl = ::gen_ctx_diff(base, dest, getter, component_type);

  if (diff_rsl.has_value()) {
    auto diff = diff_rsl.move();
    if (diff.is_left()) {
      mut_diff->upsert(diff.left_move());
    } else {
      mut_diff->delete_ctx_component(diff.get_right());
    }
  }
}

// Other utils
template <typename T>
Maybe<T> extract(uint32_t nsid, const std::unordered_map<uint32_t, T>* map,
                 const std::set<uint32_t>* alive) {
  if (alive->count(nsid) == 0u) {
    return empty_maybe{};
  }
  auto it = map->find(nsid);
  if (it == map->end()) {
    return empty_maybe{};
  }
  return it->second;
}

}  // namespace

CommonLogicSnapshot::CommonLogicSnapshot() {}

CommonLogicDiff CommonLogicSnapshot::CreateDiff(
    const CommonLogicSnapshot& base, const CommonLogicSnapshot& dest) {
  CommonLogicDiff diff{};

  // Context component diffs
  ::diff_ctx_component(base, dest, &diff, &CommonLogicSnapshot::sim_time,
                       common::pb::CtxComponentType::CCT_SIM_TIME);

  // Upsert all elements found in the base snapshot, unless they are identical
  //  in the destination snapshot. This upsertion may include deleting
  //  components.
  for (uint32_t dest_net_sync_id : dest.alive_entities_) {
    ::diff_component(dest_net_sync_id, base, dest, &diff,
                     &CommonLogicSnapshot::map_location,
                     common::pb::ComponentType::CT_MAP_LOCATION);
    ::diff_component(dest_net_sync_id, base, dest, &diff,
                     &CommonLogicSnapshot::orientation,
                     common::pb::ComponentType::CT_ORIENTATION);
    ::diff_component(dest_net_sync_id, base, dest, &diff,
                     &CommonLogicSnapshot::nav_waypoint_list,
                     common::pb::ComponentType::CT_NAV_WAYPOINTS);
    ::diff_component(dest_net_sync_id, base, dest, &diff,
                     &CommonLogicSnapshot::standard_navigation_params,
                     common::pb::ComponentType::CT_STANDARD_NAVIGATION_PARAMS);
  }

  // Find all the entities in the base snapshot that do not exist in the
  //  destination, and delete them.
  for (uint32_t base_nsid : base.alive_entities_) {
    if (dest.alive_entities_.count(base_nsid) == 0) {
      diff.delete_entity(base_nsid);
    }
  }

  return diff;
}

CommonLogicSnapshot CommonLogicSnapshot::ApplyDiff(
    const CommonLogicSnapshot& base, const CommonLogicDiff& diff) {
  CommonLogicSnapshot dest = base;

  // Upsert context components
  diff.sim_time().if_present([&dest](const auto& v) { dest.set(v); });

  // Delete context components
  for (auto deleted_ctx_component : diff.deleted_ctx_components()) {
    switch (deleted_ctx_component) {
      case common::pb::CtxComponentType::CCT_SIM_TIME:
        dest.sim_time_ = empty_maybe{};
        break;
      default:
        Logger::err(kLogLabel)
            << "Unrecognized ctx type: " << (uint32_t)deleted_ctx_component;
    }
  }

  for (uint32_t nsid : diff.upserted_entities()) {
    // Upsert components...
    diff.map_location(nsid).if_present(
        [&dest, nsid](const auto& v) { dest.add(nsid, v); });
    diff.orientation(nsid).if_present(
        [&dest, nsid](const auto& v) { dest.add(nsid, v); });
    diff.nav_waypoint_list(nsid).if_present(
        [&dest, nsid](const auto& v) { dest.add(nsid, v); });
    diff.standard_navigation_params(nsid).if_present(
        [&dest, nsid](const auto& v) { dest.add(nsid, v); });

    // Delete components...
    for (auto deleted_component_type : diff.deleted_components(nsid)) {
      switch (deleted_component_type) {
        case common::pb::ComponentType::CT_MAP_LOCATION:
          dest.nav_waypoints_.erase(nsid);
          break;
        case common::pb::ComponentType::CT_ORIENTATION:
          dest.orientations_.erase(nsid);
          break;
        case common::pb::ComponentType::CT_NAV_WAYPOINTS:
          dest.nav_waypoints_.erase(nsid);
          break;
        case common::pb::ComponentType::CT_STANDARD_NAVIGATION_PARAMS:
          dest.nav_params_.erase(nsid);
          break;

        default:
          Logger::err(kLogLabel)
              << "Unrecognized ctx type " << (uint32_t)deleted_component_type
              << " on entity #" << nsid;
      }
    }
  }

  // Delete entities that are no longer present in the new snapshot
  // (Be sure to delete all component types!!)
  for (uint32_t deleted_entity : diff.deleted_entities()) {
    dest.alive_entities_.erase(deleted_entity);
    dest.map_locations_.erase(deleted_entity);
    dest.orientations_.erase(deleted_entity);
    dest.nav_waypoints_.erase(deleted_entity);
    dest.nav_params_.erase(deleted_entity);
  }

  return dest;
}

// TODO (sessamekesh): Continue here with snapshot code!
void CommonLogicSnapshot::set(CtxSimTime sim_time) { sim_time_ = sim_time; }

void CommonLogicSnapshot::add(uint32_t net_sync_id,
                              MapLocationComponent value) {
  alive_entities_.insert(net_sync_id);
  map_locations_[net_sync_id] = value;
}

void CommonLogicSnapshot::add(uint32_t net_sync_id,
                              OrientationComponent value) {
  alive_entities_.insert(net_sync_id);
  orientations_[net_sync_id] = value;
}

void CommonLogicSnapshot::add(uint32_t net_sync_id,
                              NavWaypointListComponent value) {
  alive_entities_.insert(net_sync_id);
  nav_waypoints_[net_sync_id] = std::move(value);
}

void CommonLogicSnapshot::add(uint32_t net_sync_id,
                              StandardNavigationParamsComponent value) {
  alive_entities_.insert(net_sync_id);
  nav_params_[net_sync_id] = value;
}

Maybe<CtxSimTime> CommonLogicSnapshot::sim_time() const { return sim_time_; }

Maybe<MapLocationComponent> CommonLogicSnapshot::map_location(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, &map_locations_, &alive_entities_);
}

Maybe<OrientationComponent> CommonLogicSnapshot::orientation(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, &orientations_, &alive_entities_);
}

Maybe<NavWaypointListComponent> CommonLogicSnapshot::nav_waypoint_list(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, &nav_waypoints_, &alive_entities_);
}

Maybe<StandardNavigationParamsComponent>
CommonLogicSnapshot::standard_navigation_params(uint32_t net_sync_id) const {
  return ::extract(net_sync_id, &nav_params_, &alive_entities_);
}

common::pb::SnapshotFull CommonLogicSnapshot::serialize() const {
  common::pb::SnapshotFull pb{};

  // Context components...
  sim_time_.if_present(
      [&pb](const auto& v) { ::serialize(pb.mutable_sim_time(), v); });

  // Serialize entities...
  for (uint32_t nsid : alive_entities_) {
    common::pb::GameEntity* e = pb.add_entities();
    e->set_net_sync_id(nsid);

    // Components...
    common::pb::ComponentData* cd = e->mutable_component_data();
    map_location(nsid).if_present(
        [cd](const auto& v) { ::serialize(cd->mutable_map_location(), v); });
    orientation(nsid).if_present(
        [cd](const auto& v) { ::serialize(cd->mutable_orientation(), v); });
    nav_waypoint_list(nsid).if_present([cd](const auto& v) {
      ::serialize(cd->mutable_nav_waypoint_list(), v);
    });
    standard_navigation_params(nsid).if_present([cd](const auto& v) {
      ::serialize(cd->mutable_standard_navigation_params(), v);
    });
  }

  return pb;
}

CommonLogicSnapshot CommonLogicSnapshot::Deserialize(
    const common::pb::SnapshotFull& pb) {
  CommonLogicSnapshot snapshot{};

  // Context components...
  if (pb.has_sim_time()) {
    snapshot.set(::deserialize(pb.sim_time()));
  }

  for (const common::pb::GameEntity& e : pb.entities()) {
    uint32_t nsid = e.net_sync_id();
    const common::pb::ComponentData& cd = e.component_data();

    if (cd.has_map_location()) {
      snapshot.add(nsid, ::deserialize(cd.map_location()));
    }
    if (cd.has_orientation()) {
      snapshot.add(nsid, ::deserialize(cd.orientation()));
    }
    if (cd.has_nav_waypoint_list()) {
      snapshot.add(nsid, ::deserialize(cd.nav_waypoint_list()));
    }
    if (cd.has_standard_navigation_params()) {
      snapshot.add(nsid, ::deserialize(cd.standard_navigation_params()));
    }
  }

  return snapshot;
}
