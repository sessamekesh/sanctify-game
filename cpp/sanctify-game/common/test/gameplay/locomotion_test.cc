#include <gtest/gtest.h>
#include <sanctify-game-common/gameplay/locomotion.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

TEST(LocomotionSystem, SetsAndGetsMapLocationCorrectly) {
  entt::registry world;

  system::LocomotionSystem locomotion_system;

  auto e1 = world.create();

  locomotion_system.attach_basic_locomotion_components(
      world, e1, glm::vec2(1.f, 2.f), 5.f);

  EXPECT_EQ(locomotion_system.get_map_position(world, e1), glm::vec2(1.f, 2.f));
}

TEST(LocomotionSystem, DoesNotUpdateWithNoWaypoints) {
  entt::registry world;

  system::LocomotionSystem locomotion_system;

  auto e1 = world.create();

  locomotion_system.attach_basic_locomotion_components(
      world, e1, glm::vec2(1.f, 2.f), 5.f);

  locomotion_system.apply_standard_locomotion(world, 1.f);

  EXPECT_EQ(locomotion_system.get_map_position(world, e1), glm::vec2(1.f, 2.f));
}

TEST(LocomotionSystem, TravelsPartWayToFirstWaypoint) {
  entt::registry world;
  system::LocomotionSystem locomotion_system;

  auto e1 = world.create();
  locomotion_system.attach_basic_locomotion_components(
      world, e1, glm::vec2(0.f, 0.f), 5.f);

  PodVector<glm::vec2> waypoints(1);
  waypoints.push_back(glm::vec2{10.f, 0.f});
  world.emplace<component::NavWaypointList>(e1, std::move(waypoints));

  locomotion_system.apply_standard_locomotion(world, 1.f);

  EXPECT_EQ(locomotion_system.get_map_position(world, e1), glm::vec2(5.f, 0.f));

  component::NavWaypointList* remaining_waypoints =
      world.try_get<component::NavWaypointList>(e1);
  ASSERT_TRUE(remaining_waypoints != nullptr);

  PodVector<glm::vec2> expected_waypoints(1);
  expected_waypoints.push_back(glm::vec2{10.f, 0.f});

  EXPECT_EQ(*remaining_waypoints,
            component::NavWaypointList{std::move(expected_waypoints)});
}

TEST(LocomotionSystem, TraversesMultipleWaypointsSmoothly) {
  entt::registry world;
  system::LocomotionSystem locomotion_system;

  auto e1 = world.create();
  locomotion_system.attach_basic_locomotion_components(
      world, e1, glm::vec2(0.f, 0.f), 5.f);

  PodVector<glm::vec2> waypoints(2);
  waypoints.push_back(glm::vec2{10.f, 0.f});
  waypoints.push_back(glm::vec2{10.f, 10.f});
  world.emplace<component::NavWaypointList>(e1, std::move(waypoints));

  locomotion_system.apply_standard_locomotion(world, 3.f);

  EXPECT_EQ(locomotion_system.get_map_position(world, e1),
            glm::vec2(10.f, 5.f));

  component::NavWaypointList* remaining_waypoints =
      world.try_get<component::NavWaypointList>(e1);
  ASSERT_TRUE(remaining_waypoints != nullptr);

  PodVector<glm::vec2> expected_waypoints(1);
  expected_waypoints.push_back(glm::vec2{10.f, 10.f});

  EXPECT_EQ(*remaining_waypoints,
            component::NavWaypointList{std::move(expected_waypoints)});
}

TEST(LocomotionSystem, FinishesLocomotionAtLastWaypoint) {
  entt::registry world;
  system::LocomotionSystem locomotion_system;

  auto e1 = world.create();
  locomotion_system.attach_basic_locomotion_components(
      world, e1, glm::vec2(0.f, 0.f), 5.f);

  PodVector<glm::vec2> waypoints(2);
  waypoints.push_back(glm::vec2{10.f, 0.f});
  waypoints.push_back(glm::vec2{10.f, 10.f});
  world.emplace<component::NavWaypointList>(e1, std::move(waypoints));

  locomotion_system.apply_standard_locomotion(world, 5.f);

  EXPECT_EQ(locomotion_system.get_map_position(world, e1),
            glm::vec2(10.f, 10.f));

  component::NavWaypointList* remaining_waypoints =
      world.try_get<component::NavWaypointList>(e1);
  ASSERT_TRUE(remaining_waypoints == nullptr);
}
