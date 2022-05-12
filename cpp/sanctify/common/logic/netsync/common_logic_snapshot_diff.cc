#include "common_logic_snapshot_diff.h"

#include <igcore/log.h>
#include <igcore/maybe.h>

#include "proto_serialize.h"

using namespace sanctify;
using namespace logic;
using namespace indigo;
using namespace core;

////////////////////////////////////////////////////
//
// HELPERS
//
////////////////////////////////////////////////////

namespace {
const char* kDiffLabel = "CommonLogicDiff";

template <typename T>
void upsert_component(
    uint32_t net_sync_id, const T& c, std::set<uint32_t>* upserted_entities_set,
    std::unordered_map<uint32_t, T>* upserted_components_map) {
  upserted_entities_set->insert(net_sync_id);
  upserted_components_map->emplace(net_sync_id, c);
}

template <typename T>
Maybe<T> extract(uint32_t net_sync_id,
                 const std::unordered_map<uint32_t, T>* map) {
  auto it = map->find(net_sync_id);
  if (it == map->end()) {
    return empty_maybe{};
  }
  return it->second;
}

}  // namespace

////////////////////////////////////////////////////
//
// DIFF IMPL
//
////////////////////////////////////////////////////

CommonLogicDiff::CommonLogicDiff() {}

void CommonLogicDiff::upsert(CtxSimTime sim_time) { sim_time_ = sim_time; }

void CommonLogicDiff::delete_ctx_component(
    common::proto::CtxComponentType ctx_component_type) {
  deleted_ctx_components_.insert(ctx_component_type);
}

void CommonLogicDiff::upsert(uint32_t net_sync_id, MapLocationComponent c) {
  ::upsert_component(net_sync_id, c, &upserted_entities_,
                     &map_location_upserts_);
}

void CommonLogicDiff::upsert(uint32_t net_sync_id, OrientationComponent c) {
  ::upsert_component(net_sync_id, c, &upserted_entities_,
                     &orientation_upserts_);
}

void CommonLogicDiff::upsert(uint32_t net_sync_id, NavWaypointListComponent c) {
  ::upsert_component(net_sync_id, c, &upserted_entities_,
                     &nav_waypoint_list_upserts_);
}

void CommonLogicDiff::upsert(uint32_t net_sync_id,
                             StandardNavigationParamsComponent c) {
  ::upsert_component(net_sync_id, c, &upserted_entities_,
                     &standard_navigation_params_upserts_);
}

void CommonLogicDiff::delete_entity(uint32_t net_sync_id) {
  deleted_entities_.insert(net_sync_id);
}

void CommonLogicDiff::delete_component(
    uint32_t net_sync_id, common::proto::ComponentType component_type) {
  upserted_entities_.insert(net_sync_id);
  auto it = deleted_components_.find(net_sync_id);
  if (it == deleted_components_.end()) {
    deleted_components_.emplace(
        net_sync_id, std::set<common::proto::ComponentType>{component_type});
  } else {
    it->second.insert(component_type);
  }
}

const std::set<uint32_t>& CommonLogicDiff::deleted_entities() const {
  return deleted_entities_;
}

const std::set<uint32_t>& CommonLogicDiff::upserted_entities() const {
  return upserted_entities_;
}

std::set<common::proto::ComponentType> CommonLogicDiff::deleted_components(
    uint32_t net_sync_id) const {
  auto it = deleted_components_.find(net_sync_id);
  if (it == deleted_components_.end()) {
    return std::set<common::proto::ComponentType>{};
  } else {
    return it->second;
  }
}

std::set<common::proto::CtxComponentType> CommonLogicDiff::deleted_ctx_components()
    const {
  return deleted_ctx_components_;
}

Maybe<CtxSimTime> CommonLogicDiff::sim_time() const { return sim_time_; }

Maybe<MapLocationComponent> CommonLogicDiff::map_location(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, &map_location_upserts_);
}

Maybe<OrientationComponent> CommonLogicDiff::orientation(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, &orientation_upserts_);
}

Maybe<NavWaypointListComponent> CommonLogicDiff::nav_waypoint_list(
    uint32_t net_sync_id) const {
  return ::extract(net_sync_id, &nav_waypoint_list_upserts_);
}

Maybe<StandardNavigationParamsComponent>
CommonLogicDiff::standard_navigation_params(uint32_t net_sync_id) const {
  return ::extract(net_sync_id, &standard_navigation_params_upserts_);
}

common::proto::SnapshotDiff CommonLogicDiff::serialize() const {
  common::proto::SnapshotDiff diff_proto{};

  // Context upserts/deletes
  if (sim_time_.has_value()) {
    ::serialize(diff_proto.mutable_upsert_sim_time(), sim_time_.get());
  }

  // Upserted entities
  for (auto nsid : upserted_entities_) {
    common::proto::EntityUpdateMask* entity_upsert_proto =
        diff_proto.add_upsert_entities();

    entity_upsert_proto->set_net_sync_id(nsid);

    // Components
    common::proto::ComponentData* components =
        entity_upsert_proto->mutable_upsert_components();

    map_location(nsid).if_present([components](const auto& v) {
      ::serialize(components->mutable_map_location(), v);
    });
    orientation(nsid).if_present([components](const auto& v) {
      ::serialize(components->mutable_orientation(), v);
    });
    nav_waypoint_list(nsid).if_present([components](const auto& v) {
      ::serialize(components->mutable_nav_waypoint_list(), v);
    });
    standard_navigation_params(nsid).if_present([components](const auto& v) {
      ::serialize(components->mutable_standard_navigation_params(), v);
    });

    // Delete dropped components
    for (auto deleted_component : deleted_components(nsid)) {
      entity_upsert_proto->add_remove_components(deleted_component);
    }
  }

  // Deleted entities
  for (auto deleted_entity : deleted_entities()) {
    diff_proto.add_remove_entities(deleted_entity);
  }

  // Finished!
  return diff_proto;
}

CommonLogicDiff CommonLogicDiff::Deserialize(
    const common::proto::SnapshotDiff& diff) {
  CommonLogicDiff diff_model{};

  // Context components
  if (diff.has_upsert_sim_time()) {
    diff_model.upsert(::deserialize(diff.upsert_sim_time()));
  }

  // Upserted entities
  for (const common::proto::EntityUpdateMask& upserted_entities :
       diff.upsert_entities()) {
    uint32_t nsid = upserted_entities.net_sync_id();

    const common::proto::ComponentData& components =
        upserted_entities.upsert_components();

    // Upserted components
    if (components.has_map_location()) {
      diff_model.upsert(nsid, ::deserialize(components.map_location()));
    }
    if (components.has_orientation()) {
      diff_model.upsert(nsid, ::deserialize(components.orientation()));
    }
    if (components.has_nav_waypoint_list()) {
      diff_model.upsert(nsid, ::deserialize(components.nav_waypoint_list()));
    }
    if (components.has_standard_navigation_params()) {
      diff_model.upsert(nsid,
                        ::deserialize(components.standard_navigation_params()));
    }

    // Deleted components
    for (int i = 0; i < upserted_entities.remove_components_size(); i++) {
      diff_model.delete_component(nsid, upserted_entities.remove_components(i));
    }
  }

  // Removed entities
  for (uint32_t removed_entity : diff.remove_entities()) {
    diff_model.delete_entity(removed_entity);
  }

  // Finished!
  return diff_model;
}
