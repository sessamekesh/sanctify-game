#include "common_logic_snapshot_diff.h"

#include <gtest/gtest.h>

using namespace sanctify;
using namespace logic;

namespace {
NavWaypointListComponent build_test_nav_waypoint_component() {
  indigo::core::PodVector<glm::vec2> waypoints(2);
  waypoints.push_back(glm::vec2(5.f, 0.f));
  waypoints.push_back(glm::vec2(5.f, 10.f));
  return NavWaypointListComponent{std::move(waypoints)};
}
}  // namespace

TEST(CommonLogicDiff, SerializesAllComponents) {
  CommonLogicDiff sender{};

  // Context component setup
  CtxSimTime sim_time{1.5f};

  sender.upsert(sim_time);

  // Component setup
  MapLocationComponent map_location{glm::vec2{1.f, 2.f}};
  OrientationComponent orientation{4.f};
  NavWaypointListComponent nav_waypoints =
      ::build_test_nav_waypoint_component();
  StandardNavigationParamsComponent nav_params{5.f};

  sender.upsert(1, map_location);
  sender.upsert(1, orientation);
  sender.upsert(1, nav_waypoints);
  sender.upsert(1, nav_params);

  // Serialize and deserialize!
  CommonLogicDiff receiver = CommonLogicDiff::Deserialize(sender.serialize());

  // Context component assertions
  EXPECT_EQ(receiver.sim_time(), sim_time);

  // Component assertions
  EXPECT_EQ(receiver.map_location(1), map_location);
  EXPECT_EQ(receiver.orientation(1), orientation);
  EXPECT_EQ(receiver.nav_waypoint_list(1), nav_waypoints);
  EXPECT_EQ(receiver.standard_navigation_params(1), nav_params);
}

TEST(CommonLogicDiff, SeparatesEntitiesById) {
  CommonLogicDiff sender{};

  sender.upsert(CtxSimTime{1.5f});

  sender.upsert(1, MapLocationComponent{glm::vec2{1.f, 2.f}});
  sender.upsert(2, OrientationComponent{3.f});

  sender.delete_entity(4);

  sender.delete_component(
      5, common::proto::ComponentType::CT_STANDARD_NAVIGATION_PARAMS);

  ASSERT_TRUE(sender.sim_time().has_value());
  EXPECT_FLOAT_EQ(sender.sim_time().get().simTimeSeconds, 1.5f);

  EXPECT_TRUE(sender.map_location(1).has_value());
  EXPECT_FALSE(sender.map_location(2).has_value());
  EXPECT_FALSE(sender.orientation(1).has_value());

  EXPECT_TRUE(sender.orientation(2).has_value());
  EXPECT_FALSE(sender.map_location(2).has_value());

  EXPECT_EQ(sender.deleted_entities(), std::set<uint32_t>{4});

  EXPECT_EQ(sender.deleted_components(5),
            std::set<common::proto::ComponentType>{
                common::proto::ComponentType::CT_STANDARD_NAVIGATION_PARAMS});

  CommonLogicDiff receiver = CommonLogicDiff::Deserialize(sender.serialize());

  ASSERT_TRUE(receiver.sim_time().has_value());
  EXPECT_FLOAT_EQ(receiver.sim_time().get().simTimeSeconds, 1.5f);

  EXPECT_TRUE(receiver.map_location(1).has_value());
  EXPECT_FALSE(receiver.map_location(2).has_value());
  EXPECT_FALSE(receiver.orientation(1).has_value());

  EXPECT_TRUE(receiver.orientation(2).has_value());
  EXPECT_FALSE(receiver.map_location(2).has_value());

  EXPECT_EQ(receiver.deleted_entities(), std::set<uint32_t>{4});

  EXPECT_EQ(receiver.deleted_components(5),
            std::set<common::proto::ComponentType>{
                common::proto::ComponentType::CT_STANDARD_NAVIGATION_PARAMS});
}
