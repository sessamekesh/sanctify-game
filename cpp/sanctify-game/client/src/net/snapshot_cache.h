#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_NET_SNAPSHOT_CACHE_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_NET_SNAPSHOT_CACHE_H

#include <igcore/maybe.h>
#include <sanctify-game-common/net/game_snapshot.h>

#include <map>

/**
 * SnapshotCache - keeps a record of snapshots received from the server, with
 *  (estimated) cleanup for old snapshots to avoid huge memory footprint.
 */

namespace sanctify {

class SnapshotCache {
 public:
  SnapshotCache(uint32_t buffer_size);

  void store_server_snapshot(GameSnapshot snapshot);
  indigo::core::Maybe<GameSnapshot> assemble_diff_and_store_snapshot(
      const GameSnapshotDiff& diff);

  uint32_t most_recent_snapshot_id() const;

 private:
  uint32_t snapshot_buffer_count_;
  std::map<uint32_t, GameSnapshot> stored_snapshots_;
  indigo::core::PodVector<uint32_t> base_snapshots_;

  uint32_t most_recent_snapshot_id_;
};

}  // namespace sanctify

#endif
