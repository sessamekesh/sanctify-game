#include "locomotion_system.h"

#include <common/logic/update_common/tick_time_elapsed.h>
#include <gtest/gtest.h>

#include <glm/gtc/constants.hpp>

using namespace sanctify;
using namespace logic;
using namespace indigo;
using namespace core;
using namespace igecs;

TEST(LocomotionSystem_Update, NoWaypoints) {
  entt::registry world;
  WorldView thin_view = WorldView::Thin(&world);

  auto e = world.create();
  LocomotionUtil::attach_locomotion_components(&thin_view, e,
                                               glm::vec2{1.f, 2.f}, 5.f);

  FrameTimeElapsedUtil::mark_time_elapsed(&thin_view, 1.f);
  WorldView wv = LocomotionSystem::decl().create(&world);
  LocomotionSystem::update(&wv);

  EXPECT_EQ(LocomotionUtil::get_map_position(&thin_view, e),
            glm::vec2(1.f, 2.f));
}

TEST(LocomotionSystem_Update, TravelsPartWayToFirstWaypoint) {
  entt::registry world;
  WorldView thin_view = WorldView::Thin(&world);

  auto e = world.create();
  LocomotionUtil::attach_locomotion_components(&thin_view, e,
                                               glm::vec2{0.f, 0.f}, 5.f);
  PodVector<glm::vec2> waypoints(1);
  waypoints.push_back(glm::vec2(10.f, 0.f));
  LocomotionUtil::set_waypoints(&thin_view, e, std::move(waypoints));

  FrameTimeElapsedUtil::mark_time_elapsed(&thin_view, 1.f);
  WorldView wv = LocomotionSystem::decl().create(&world);
  LocomotionSystem::update(&wv);

  EXPECT_EQ(LocomotionUtil::get_map_position(&thin_view, e),
            glm::vec2(5.f, 0.f));

  NavWaypointListComponent* remaining_waypoints =
      world.try_get<NavWaypointListComponent>(e);
  ASSERT_TRUE(remaining_waypoints != nullptr);

  PodVector<glm::vec2> expected_waypoints(1);
  expected_waypoints.push_back(glm::vec2(10.f, 0.f));

  EXPECT_EQ(*remaining_waypoints,
            NavWaypointListComponent{std::move(expected_waypoints)});
}

TEST(LocomotionSystem, TraversesMultipleWaypointsSmoothly) {
  entt::registry world;
  WorldView thin_view = WorldView::Thin(&world);

  auto e = world.create();
  LocomotionUtil::attach_locomotion_components(&thin_view, e,
                                               glm::vec2{0.f, 0.f}, 5.f);
  PodVector<glm::vec2> waypoints(1);
  waypoints.push_back(glm::vec2{10.f, 0.f});
  waypoints.push_back(glm::vec2{10.f, 10.f});
  LocomotionUtil::set_waypoints(&thin_view, e, std::move(waypoints));

  FrameTimeElapsedUtil::mark_time_elapsed(&thin_view, 3.f);
  WorldView wv = LocomotionSystem::decl().create(&world);
  LocomotionSystem::update(&wv);

  EXPECT_EQ(LocomotionUtil::get_map_position(&thin_view, e),
            glm::vec2(10.f, 5.f));

  NavWaypointListComponent* remaining_waypoints =
      world.try_get<NavWaypointListComponent>(e);
  ASSERT_TRUE(remaining_waypoints != nullptr);

  PodVector<glm::vec2> expected_waypoints(1);
  expected_waypoints.push_back(glm::vec2{10.f, 10.f});

  EXPECT_EQ(*remaining_waypoints,
            NavWaypointListComponent{std::move(expected_waypoints)});
}

TEST(LocomotionSystem, FinishesLocomotionAtLastWaypoint) {
  entt::registry world;
  WorldView thin_view = WorldView::Thin(&world);

  auto e = world.create();
  LocomotionUtil::attach_locomotion_components(&thin_view, e,
                                               glm::vec2{0.f, 0.f}, 5.f);
  PodVector<glm::vec2> waypoints(1);
  waypoints.push_back(glm::vec2{10.f, 0.f});
  waypoints.push_back(glm::vec2{10.f, 10.f});
  LocomotionUtil::set_waypoints(&thin_view, e, std::move(waypoints));

  FrameTimeElapsedUtil::mark_time_elapsed(&thin_view, 5.f);
  WorldView wv = LocomotionSystem::decl().create(&world);
  LocomotionSystem::update(&wv);

  EXPECT_EQ(LocomotionUtil::get_map_position(&thin_view, e),
            glm::vec2(10.f, 10.f));

  NavWaypointListComponent* remaining_waypoints =
      world.try_get<NavWaypointListComponent>(e);
  ASSERT_TRUE(remaining_waypoints == nullptr);
}

TEST(LocomotionSystem, UpdatesOrientationOnWalk) {
  entt::registry world;
  WorldView thin_view = WorldView::Thin(&world);

  auto e = world.create();
  LocomotionUtil::attach_locomotion_components(&thin_view, e,
                                               glm::vec2{0.f, 0.f}, 5.f, 2.f);
  PodVector<glm::vec2> waypoints(1);
  waypoints.push_back(glm::vec2{10.f, 0.f});
  waypoints.push_back(glm::vec2{10.f, 10.f});
  LocomotionUtil::set_waypoints(&thin_view, e, std::move(waypoints));

  WorldView wv = LocomotionSystem::decl().create(&world);

  // Before movement: should be whatever it was set to before
  EXPECT_EQ(wv.read<OrientationComponent>(e).orientation, 2.f);

  // Move a bit, expect an update
  FrameTimeElapsedUtil::mark_time_elapsed(&thin_view, 1.f);
  LocomotionSystem::update(&wv);
  EXPECT_EQ(wv.read<OrientationComponent>(e).orientation,
            glm::half_pi<float>());

  // Move to another waypoint, another update
  FrameTimeElapsedUtil::mark_time_elapsed(&thin_view, 2.f);
  LocomotionSystem::update(&wv);
  EXPECT_EQ(wv.read<OrientationComponent>(e).orientation, 0.f);

  // Expect it to stay the same after finishing
  FrameTimeElapsedUtil::mark_time_elapsed(&thin_view, 2.f);
  LocomotionSystem::update(&wv);
  EXPECT_EQ(wv.read<OrientationComponent>(e).orientation, 0.f);

  // Expect it to not update if there's no movement on this component
  wv.write<OrientationComponent>(e).orientation = 2.f;
  FrameTimeElapsedUtil::mark_time_elapsed(&thin_view, 2.f);
  LocomotionSystem::update(&wv);
  EXPECT_EQ(wv.read<OrientationComponent>(e).orientation, 2.f);
}
