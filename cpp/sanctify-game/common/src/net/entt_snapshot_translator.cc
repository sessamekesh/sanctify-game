#include <sanctify-game-common/gameplay/net_sync_components.h>
#include <sanctify-game-common/net/entt_snapshot_translator.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {

template <typename T>
void maybe_add_entity(entt::entity entity, entt::registry& world,
                      Maybe<T> component) {
  if (component.has_value()) {
    auto& c = world.emplace<T>(entity);
    c = component.move();
  }
}

template <typename T>
void maybe_add_tag(entt::entity entity, entt::registry& world, Maybe<T> tag) {
  if (tag.has_value()) {
    world.emplace<T>(entity);
  }
}

}  // namespace

void EnttSnapshotTranslator::write_fresh_game_state(
    entt::registry& world, const GameSnapshot& snapshot) {
  world.clear();

  for (auto net_sync_id : snapshot.alive_entities()) {
    auto entity = world.create();

    world.emplace<component::NetSyncId>(entity, net_sync_id);

    ::maybe_add_entity(entity, world, snapshot.map_location(net_sync_id));
    ::maybe_add_entity(entity, world, snapshot.nav_waypoint_list(net_sync_id));
    ::maybe_add_entity(entity, world,
                       snapshot.standard_navigation_params(net_sync_id));
    ::maybe_add_tag(entity, world,
                    snapshot.basic_player_component(net_sync_id));
    ::maybe_add_entity(entity, world, snapshot.orientation(net_sync_id));
  }
}

EnttSnapshotTranslator::ReadAllGameStateResult
EnttSnapshotTranslator::read_all_game_state(entt::registry& world,
                                            uint32_t snapshot_id,
                                            float simulation_time) {
  GameSnapshot snapshot{};
  snapshot.snapshot_time(simulation_time);
  snapshot.snapshot_id(snapshot_id);

  Bimap<uint32_t, entt::entity> entityBimap;

  auto view = world.view<const component::NetSyncId>();

  for (auto [entity, net_sync] : view.each()) {
    component::MapLocation* map_location =
        world.try_get<component::MapLocation>(entity);
    component::NavWaypointList* nav_waypoint =
        world.try_get<component::NavWaypointList>(entity);
    component::StandardNavigationParams* nav_params =
        world.try_get<component::StandardNavigationParams>(entity);
    bool has_basic_player_component =
        world.all_of<component::BasicPlayerComponent>(entity);
    component::OrientationComponent* orientation =
        world.try_get<component::OrientationComponent>(entity);

    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(map_location));
    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(nav_waypoint));
    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(nav_params));
    snapshot.add(net_sync.Id, has_basic_player_component
                                  ? Maybe<component::BasicPlayerComponent>(
                                        component::BasicPlayerComponent{})
                                  : empty_maybe{});
    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(orientation));

    entityBimap.insert(net_sync.Id, entity);
  }

  return {snapshot, entityBimap};
}
