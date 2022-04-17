#include "common_logic_snapshot.h"

#include <gtest/gtest.h>

using namespace sanctify;
using namespace logic;
using namespace indigo;
using namespace core;

namespace {
NavWaypointListComponent build_test_nav_waypoint_component() {
  indigo::core::PodVector<glm::vec2> waypoints(2);
  waypoints.push_back(glm::vec2(5.f, 0.f));
  waypoints.push_back(glm::vec2(5.f, 10.f));
  return NavWaypointListComponent{std::move(waypoints)};
}
}  // namespace

TEST(CommonLogicSnapshot, SerializesAllComponents) {
  CommonLogicSnapshot snapshot{};

  // Context component setup
  CtxSimTime sim_time{1.5f};

  snapshot.set(sim_time);

  // Component setup
  MapLocationComponent map_location{glm::vec2{1.f, 2.f}};
  OrientationComponent orientation{4.f};
  NavWaypointListComponent nav_waypoints =
      ::build_test_nav_waypoint_component();
  StandardNavigationParamsComponent nav_params{5.f};

  snapshot.add(1, map_location);
  snapshot.add(1, orientation);
  snapshot.add(1, nav_waypoints);
  snapshot.add(1, nav_params);

  // Serialize and deserialize!
  CommonLogicSnapshot receiver =
      CommonLogicSnapshot::Deserialize(snapshot.serialize());

  // Context component assertions
  EXPECT_EQ(receiver.sim_time(), sim_time);

  // Component assertions
  EXPECT_EQ(receiver.map_location(1), map_location);
  EXPECT_EQ(receiver.orientation(1), orientation);
  EXPECT_EQ(receiver.nav_waypoint_list(1), nav_waypoints);
  EXPECT_EQ(receiver.standard_navigation_params(1), nav_params);
}

TEST(CommonLogicSnapshot, ProducesCorrectDiff) {
  CommonLogicSnapshot base{}, dest{};

  // Context - sim time was changed
  base.set(CtxSimTime{1.f});
  dest.set(CtxSimTime{1.5f});

  // Entity 1 - has an updated StandardNavigationParams, that's it
  base.add(1, MapLocationComponent{glm::vec2(1.f, 2.f)});
  base.add(1, StandardNavigationParamsComponent{5.f});
  dest.add(1, MapLocationComponent{glm::vec2{1.f, 2.f}});
  dest.add(1, StandardNavigationParamsComponent{6.f});

  // Entity 2 - deleted NavWaypointList
  base.add(2, ::build_test_nav_waypoint_component());
  base.add(2, StandardNavigationParamsComponent{5.f});
  dest.add(2, StandardNavigationParamsComponent{5.f});

  // Entity 3 - newly added entity
  dest.add(3, MapLocationComponent{glm::vec2(2.f, 4.f)});

  // Entity 4 - fully deleted
  base.add(4, ::build_test_nav_waypoint_component());
  base.add(4, StandardNavigationParamsComponent{5.f});

  CommonLogicDiff diff = CommonLogicSnapshot::CreateDiff(base, dest);

  EXPECT_EQ(diff.sim_time(), CtxSimTime{1.5f});

  EXPECT_EQ(diff.deleted_components(1), std::set<common::pb::ComponentType>{});
  EXPECT_EQ(diff.deleted_components(2),
            std::set<common::pb::ComponentType>{
                common::pb::ComponentType::CT_NAV_WAYPOINTS});
  EXPECT_EQ(diff.deleted_components(3), std::set<common::pb::ComponentType>{});
  EXPECT_EQ(diff.deleted_components(4), std::set<common::pb::ComponentType>{});

  // Entity 1 should only have StandardNavigationParams - nothing else changed
  EXPECT_TRUE(diff.map_location(1).is_empty());
  EXPECT_EQ(diff.standard_navigation_params(1),
            StandardNavigationParamsComponent{6.f});

  // Entity 2 should not have a StandardNavigationParams (it didn't change)
  EXPECT_TRUE(diff.standard_navigation_params(2).is_empty());

  // Entity 3 should be present with MapLocation
  EXPECT_EQ(diff.map_location(3), MapLocationComponent{glm::vec2(2.f, 4.f)});

  // Entity 4 should not have NavWaypointList or StandardNavParams
  EXPECT_TRUE(diff.standard_navigation_params(4).is_empty());
  EXPECT_TRUE(diff.nav_waypoint_list(4).is_empty());
}

TEST(CommonLogicSnapshot, AppliesDiffCorrectly) {
  CommonLogicSnapshot base{};
  CommonLogicDiff diff{};

  // Entity 1 - location updates, but nothing else does
  base.add(1, MapLocationComponent{glm::vec2(1.f, 2.f)});
  base.add(1, StandardNavigationParamsComponent{5.f});
  diff.upsert(1, MapLocationComponent{glm::vec2(2.f, 4.f)});

  // Entity 2 - waypoints are deleted
  base.add(2, MapLocationComponent{glm::vec2(1.f, 2.f)});
  base.add(2, ::build_test_nav_waypoint_component());
  diff.delete_component(2, common::pb::ComponentType::CT_NAV_WAYPOINTS);

  // Entity 3 is not changed
  base.add(3, MapLocationComponent{glm::vec2(1.f, 2.f)});

  // Entity 4 is deleted
  base.add(4, StandardNavigationParamsComponent{5.f});
  diff.delete_entity(4);

  CommonLogicSnapshot dest = CommonLogicSnapshot::ApplyDiff(base, diff);

  // Entity 1 should have new location
  EXPECT_EQ(dest.map_location(1), MapLocationComponent{glm::vec2(2.f, 4.f)});
  EXPECT_EQ(dest.standard_navigation_params(1),
            StandardNavigationParamsComponent{5.f});

  // Entity 2 should no longer have waypoints
  EXPECT_EQ(dest.map_location(2), MapLocationComponent{glm::vec2(1.f, 2.f)});
  EXPECT_TRUE(dest.nav_waypoint_list(2).is_empty());

  // Entity 3 should be unchanged
  EXPECT_EQ(dest.map_location(3), MapLocationComponent{glm::vec2(1.f, 2.f)});

  // Entity 4 should no longer be present
  EXPECT_TRUE(dest.standard_navigation_params(4).is_empty());
}
