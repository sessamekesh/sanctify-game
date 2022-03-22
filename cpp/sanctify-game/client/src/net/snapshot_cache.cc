#include <net/snapshot_cache.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "SnapshotCache";
}

SnapshotCache::SnapshotCache(uint32_t buffer_size)
    : snapshot_buffer_count_(buffer_size),
      base_snapshots_(buffer_size),
      most_recent_snapshot_id_(0u) {}

void SnapshotCache::store_server_snapshot(GameSnapshot snapshot) {
  uint32_t snapshot_id = snapshot.snapshot_id();

  if (base_snapshots_.size() == 0) {
    base_snapshots_.push_back(snapshot_id);
    stored_snapshots_.emplace(snapshot_id, std::move(snapshot));
    return;
  }

  // Skip snapshots that are older than the last tolerable ID (this should be
  // rare)
  if (base_snapshots_[0] >= snapshot_id) {
    return;
  }

  // Ignore duplicates
  if (base_snapshots_.contains(snapshot_id)) {
    return;
  }

  // Replace oldest snapshot ID with this snapshot, and delete all older
  // snapshots that the new oldest tolerable, and register this snapshot
  if (base_snapshots_.size() < snapshot_buffer_count_) {
    base_snapshots_.push_back(snapshot_id);
  } else {
    base_snapshots_[0] = snapshot_id;
  }
  base_snapshots_.in_place_sort();

  for (auto it = stored_snapshots_.begin(); it != stored_snapshots_.end();
       ++it) {
    if (it->first < base_snapshots_[0]) {
      it = stored_snapshots_.erase(it);
      if (it == stored_snapshots_.end()) {
        break;
      }
    }
  }

  stored_snapshots_.emplace(snapshot_id, std::move(snapshot));
}

Maybe<GameSnapshot> SnapshotCache::assemble_diff_and_store_snapshot(
    const GameSnapshotDiff& diff) {
  auto it = stored_snapshots_.find(diff.base_snapshot_id());
  if (it == stored_snapshots_.end()) {
    Logger::err(kLogLabel)
        << "Received snapshot against a base that's been deleted - "
        << diff.base_snapshot_id();
    return empty_maybe{};
  }

  const GameSnapshot& base_snapshot = it->second;

  auto dest_it = stored_snapshots_.find(diff.dest_snapshot_id());
  if (dest_it != stored_snapshots_.end()) {
    return dest_it->second;
  }

  GameSnapshot new_snapshot = GameSnapshot::ApplyDiff(it->second, diff);
  stored_snapshots_.emplace(diff.dest_snapshot_id(), new_snapshot);
  if (new_snapshot.snapshot_id() > most_recent_snapshot_id_) {
    most_recent_snapshot_id_ = new_snapshot.snapshot_id();
  }

  return new_snapshot;
}

uint32_t SnapshotCache::most_recent_snapshot_id() const {
  return most_recent_snapshot_id_;
}
