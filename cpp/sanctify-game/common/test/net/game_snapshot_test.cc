#include <gtest/gtest.h>
#include <sanctify-game-common/net/game_snapshot.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
PodVector<glm::vec2> create_test_waypoints(int count) {
  PodVector<glm::vec2> waypoints(count);

  for (int i = 1; i <= count; i++) {
    waypoints.push_back(glm::vec2{i * 2.f, i * 4.f});
  }

  return waypoints;
}
}  // namespace

TEST(GameSnapshotDiff, UpsertMultipleComponents) {
  GameSnapshotDiff diff;

  diff.upsert(1, component::MapLocation{glm::vec2{2.f, 4.f}});
  diff.upsert(1, component::StandardNavigationParams{5.f});
  diff.upsert(1, component::NavWaypointList{::create_test_waypoints(2)});

  Maybe<component::MapLocation> map_location = diff.map_location(1);
  Maybe<component::StandardNavigationParams> nav_params =
      diff.standard_navigation_params(1);
  Maybe<component::NavWaypointList> waypoint_list = diff.nav_waypoint_list(1);

  ASSERT_TRUE(map_location.has_value());
  ASSERT_TRUE(nav_params.has_value());
  ASSERT_TRUE(waypoint_list.has_value());

  EXPECT_EQ(map_location.get().XZ.x, 2.f);
  EXPECT_EQ(map_location.get().XZ.y, 4.f);

  EXPECT_EQ(nav_params.get().MovementSpeed, 5.f);

  ASSERT_EQ(waypoint_list.get().Targets.size(), 2);
  EXPECT_EQ(waypoint_list.get().Targets[0].x, 2.f);
  EXPECT_EQ(waypoint_list.get().Targets[0].y, 4.f);
  EXPECT_EQ(waypoint_list.get().Targets[1].x, 4.f);
  EXPECT_EQ(waypoint_list.get().Targets[1].y, 8.f);

  EXPECT_TRUE(diff.map_location(2).is_empty());
  EXPECT_TRUE(diff.standard_navigation_params(2).is_empty());
  EXPECT_TRUE(diff.nav_waypoint_list(2).is_empty());

  EXPECT_EQ(diff.upserted_entities().size(), 1);
  EXPECT_EQ(diff.deleted_entities().size(), 0);
}

TEST(GameSnapshotDiff, DeletedEntities) {
  GameSnapshotDiff diff;

  diff.delete_component(1, GameSnapshotDiff::ComponentType::MapLocation);
  diff.delete_entity(2);

  ASSERT_EQ(diff.deleted_entities().size(), 1);
  ASSERT_EQ(diff.deleted_entities()[0], 2);

  ASSERT_EQ(diff.upserted_entities().size(), 1);

  EXPECT_EQ(diff.deleted_components(2).size(), 0);
  ASSERT_EQ(diff.deleted_components(1).size(), 1);
  EXPECT_EQ(diff.deleted_components(1)[0],
            GameSnapshotDiff::ComponentType::MapLocation);
}

// Testing serialization and deserialization independently will be a horrible
// thing to maintain, and really we're not terribly concerned with the internals
// of the proto - we just care that the diff can be successfully saved and
// restored!
TEST(GameSnapshotDiff, TestSerializeAndDeserialize) {
  GameSnapshotDiff diff;

  // Basic elements (non component specific)
  diff.snapshot_time(100.f);
  diff.base_snapshot_id(123);
  diff.dest_snapshot_id(125);
  diff.delete_entity(1);
  diff.delete_entity(2);

  // ADD COMPONENT DELETIONS
  diff.delete_component(3, GameSnapshotDiff::ComponentType::MapLocation);
  diff.delete_component(3, GameSnapshotDiff::ComponentType::NavWaypointList);
  diff.delete_component(
      3, GameSnapshotDiff::ComponentType::StandardNavigationParams);
  diff.delete_component(3,
                        GameSnapshotDiff::ComponentType::BasicPlayerComponent);
  diff.delete_component(3, GameSnapshotDiff::ComponentType::Orientation);

  // ADD COMPONENTS
  diff.upsert(4, component::MapLocation{glm::vec2(1.f, 2.f)});
  diff.upsert(4, component::NavWaypointList{::create_test_waypoints(2)});
  diff.upsert(4, component::StandardNavigationParams{5.f});
  diff.upsert(4, component::BasicPlayerComponent{});
  diff.upsert(4, component::OrientationComponent{1.f});

  // Serialize into a proto, and deserialize into a new GameSnapshotDiff object
  pb::GameSnapshotDiff proto = diff.serialize();

  GameSnapshotDiff gen_diff = GameSnapshotDiff::Deserialize(proto);

  // Basic assertions (non component specific)
  EXPECT_EQ(gen_diff.snapshot_time(), 100.f);
  EXPECT_EQ(gen_diff.base_snapshot_id(), 123);
  EXPECT_EQ(gen_diff.dest_snapshot_id(), 125);

  EXPECT_EQ(gen_diff.deleted_entities().size(), 2);
  EXPECT_EQ(gen_diff.deleted_entities()[0], 1);
  EXPECT_EQ(gen_diff.deleted_entities()[1], 2);
  EXPECT_EQ(gen_diff.base_snapshot_id(), 123);
  EXPECT_EQ(gen_diff.dest_snapshot_id(), 125);

  PodVector<GameSnapshotDiff::ComponentType> deleted_components =
      gen_diff.deleted_components(3);
  EXPECT_FALSE(
      deleted_components.contains(GameSnapshotDiff::ComponentType::Invalid));

  // VERIFY COMPONENT DELETIONS
  EXPECT_TRUE(deleted_components.contains(
      GameSnapshotDiff::ComponentType::MapLocation));
  EXPECT_TRUE(deleted_components.contains(
      GameSnapshotDiff::ComponentType::NavWaypointList));
  EXPECT_TRUE(deleted_components.contains(
      GameSnapshotDiff::ComponentType::StandardNavigationParams));
  EXPECT_TRUE(deleted_components.contains(
      GameSnapshotDiff::ComponentType::BasicPlayerComponent));
  EXPECT_TRUE(deleted_components.contains(
      GameSnapshotDiff::ComponentType::Orientation));

  // VERIFY NEW COMPONENTS
  EXPECT_EQ(gen_diff.map_location(4),
            (component::MapLocation{glm::vec2{1.f, 2.f}}));
  EXPECT_EQ(gen_diff.nav_waypoint_list(4),
            component::NavWaypointList{::create_test_waypoints(2)});
  EXPECT_EQ(gen_diff.standard_navigation_params(4),
            component::StandardNavigationParams{5.f});
  EXPECT_EQ(gen_diff.basic_player_component(4),
            component::BasicPlayerComponent{});
  EXPECT_EQ(gen_diff.orientation(4), component::OrientationComponent{1.f});
}

TEST(GameSnapshot, TestSerializeAndDeserialize) {
  GameSnapshot snapshot{};
  snapshot.snapshot_time(100.f);

  // ADD COMPONENTS
  snapshot.add(1, component::MapLocation{glm::vec2(1.f, 2.f)});
  snapshot.add(1, component::NavWaypointList{::create_test_waypoints(2)});
  snapshot.add(1, component::StandardNavigationParams{5.f});
  snapshot.add(1, component::BasicPlayerComponent{});
  snapshot.add(1, component::OrientationComponent{1.f});

  // Perform serialization -> deserialization...
  pb::GameSnapshotFull pb = snapshot.serialize();

  GameSnapshot gen_snapshot = GameSnapshot::Deserialize(pb);

  EXPECT_EQ(gen_snapshot.snapshot_time(), 100.f);

  // VERIFY COMPONENTS
  EXPECT_EQ(gen_snapshot.map_location(1),
            (component::MapLocation{glm::vec2{1.f, 2.f}}));
  EXPECT_EQ(gen_snapshot.nav_waypoint_list(1),
            component::NavWaypointList{::create_test_waypoints(2)});
  EXPECT_EQ(gen_snapshot.standard_navigation_params(1),
            component::StandardNavigationParams{5.f});
  EXPECT_EQ(gen_snapshot.basic_player_component(1),
            component::BasicPlayerComponent{});
  EXPECT_EQ(gen_snapshot.orientation(1), component::OrientationComponent{1.f});

  // VERIFY COMPONENT DELETIONS
  gen_snapshot.delete_entity(1);
  EXPECT_EQ(gen_snapshot.map_location(1), empty_maybe{});
  EXPECT_EQ(gen_snapshot.nav_waypoint_list(1), empty_maybe{});
  EXPECT_EQ(gen_snapshot.standard_navigation_params(1), empty_maybe{});
  EXPECT_EQ(gen_snapshot.basic_player_component(1), empty_maybe{});
  EXPECT_EQ(gen_snapshot.orientation(1), empty_maybe{});
}

TEST(GameSnapshot, ProducesCorrectDiff) {
  GameSnapshot base{}, dest{};
  base.snapshot_time(100.f);
  dest.snapshot_time(101.f);
  base.snapshot_id(500);
  dest.snapshot_id(501);

  // Entity 1 - has an updated StandardNavigationParams, that's it
  base.add(1, component::MapLocation{glm::vec2(1.f, 2.f)});
  base.add(1, component::StandardNavigationParams{5.f});
  dest.add(1, component::MapLocation{glm::vec2(1.f, 2.f)});
  dest.add(1, component::StandardNavigationParams{6.f});

  // Entity 2 - deleted NavWaypointList
  base.add(2, component::NavWaypointList{::create_test_waypoints(2)});
  base.add(2, component::StandardNavigationParams{5.f});
  dest.add(2, component::StandardNavigationParams{5.f});

  // Entity 3 - newly added entity
  dest.add(3, component::MapLocation{glm::vec2(2.f, 4.f)});

  // Entity 4 - fully deleted
  base.add(4, component::NavWaypointList{::create_test_waypoints(2)});
  base.add(4, component::StandardNavigationParams{5.f});

  GameSnapshotDiff diff = GameSnapshot::CreateDiff(base, dest);

  EXPECT_EQ(diff.snapshot_time(), dest.snapshot_time());
  EXPECT_EQ(diff.dest_snapshot_id(), dest.snapshot_id());
  EXPECT_EQ(diff.base_snapshot_id(), base.snapshot_id());

  // Make sure only Entity 4 was deleted in the diff
  PodVector<uint32_t> deleted_entities = diff.deleted_entities();
  EXPECT_FALSE(deleted_entities.contains(1));
  EXPECT_FALSE(deleted_entities.contains(2));
  EXPECT_FALSE(deleted_entities.contains(3));
  EXPECT_TRUE(deleted_entities.contains(4));

  // Make sure Entities 1, 3, and 4 don't have any individually deleted
  //  components, but that Entity 2 has exactly one deleted component
  //  (NavWaypointList)
  PodVector<GameSnapshotDiff::ComponentType> e1c = diff.deleted_components(1);
  EXPECT_EQ(e1c.size(), 0);

  PodVector<GameSnapshotDiff::ComponentType> e2c = diff.deleted_components(2);
  EXPECT_EQ(e2c.size(), 1);
  EXPECT_EQ(e2c[0], GameSnapshotDiff::ComponentType::NavWaypointList);

  PodVector<GameSnapshotDiff::ComponentType> e3c = diff.deleted_components(3);
  EXPECT_EQ(e3c.size(), 0);

  PodVector<GameSnapshotDiff::ComponentType> e4c = diff.deleted_components(4);
  EXPECT_EQ(e4c.size(), 0);

  // Entity 1 should only have StandardNavigationParams - nothing else changed
  EXPECT_TRUE(diff.map_location(1).is_empty());
  EXPECT_EQ(diff.standard_navigation_params(1),
            component::StandardNavigationParams{6.f});

  // Entity 2 should not have a StandardNavigationParams (it didn't change)
  EXPECT_TRUE(diff.standard_navigation_params(2).is_empty());

  // Entity 3 should be present with MapLocation
  EXPECT_EQ(diff.map_location(3), component::MapLocation{glm::vec2(2.f, 4.f)});

  // Entity 4 should not have NavWaypointList or StandardNavigationParams
  EXPECT_TRUE(diff.standard_navigation_params(4).is_empty());
  EXPECT_TRUE(diff.nav_waypoint_list(4).is_empty());
}

TEST(GameSnapshot, AppliesDiffCorrectly) {
  GameSnapshot base{};
  GameSnapshotDiff diff{};

  base.snapshot_id(10);
  base.snapshot_time(95.f);
  diff.base_snapshot_id(10);
  diff.dest_snapshot_id(20);
  diff.snapshot_time(100.f);

  // Entity 1 - location updates, but nothing else does
  base.add(1, component::MapLocation{glm::vec2(1.f, 2.f)});
  base.add(1, component::StandardNavigationParams{5.f});
  diff.upsert(1, component::MapLocation{glm::vec2(2.f, 4.f)});

  // Entity 2 - waypoints are deleted
  base.add(2, component::MapLocation{glm::vec2(1.f, 2.f)});
  base.add(2, component::NavWaypointList{::create_test_waypoints(2)});
  diff.delete_component(2, GameSnapshotDiff::ComponentType::NavWaypointList);

  // Entity 3 is not changed
  base.add(3, component::MapLocation{glm::vec2(1.f, 2.f)});

  // Entity 4 is deleted
  base.add(4, component::StandardNavigationParams{5.f});
  diff.delete_entity(4);

  GameSnapshot dest = GameSnapshot::ApplyDiff(base, diff);

  EXPECT_EQ(dest.snapshot_id(), 20);
  EXPECT_EQ(dest.snapshot_time(), 100.f);

  // Entity 1 should have new location
  EXPECT_EQ(dest.map_location(1), component::MapLocation{glm::vec2(2.f, 4.f)});
  EXPECT_EQ(dest.standard_navigation_params(1),
            component::StandardNavigationParams{5.f});

  // Entity 2 should no longer have waypoints
  EXPECT_EQ(dest.map_location(2), component::MapLocation{glm::vec2(1.f, 2.f)});
  EXPECT_TRUE(dest.nav_waypoint_list(2).is_empty());

  // Entity 3 should be unchanged
  EXPECT_EQ(dest.map_location(3), component::MapLocation{glm::vec2(1.f, 2.f)});

  // Entity 4 should no longer be present
  EXPECT_TRUE(dest.standard_navigation_params(4).is_empty());
}
