#ifndef SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_NET_SNAPSHOT_WRITER_H
#define SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_NET_SNAPSHOT_WRITER_H

#include <igcore/bimap.h>
#include <sanctify-game-common/net/game_snapshot.h>

#include <entt/entt.hpp>

namespace sanctify {

class EnttSnapshotTranslator {
 public:
  struct ReadAllGameStateResult {
    GameSnapshot gameSnapshot;
    indigo::core::Bimap<uint32_t, entt::entity> entityBimap;
  };

 public:
  // Write a GameSnapshot object to a fresh world - clear out the registry, and
  // write entirely new entities for everything (fresh new states)
  void write_fresh_game_state(entt::registry& world,
                              const GameSnapshot& snapshot);

  ReadAllGameStateResult read_all_game_state(entt::registry& world,
                                             uint32_t snapshot_id,
                                             float world_time);
};

}  // namespace sanctify

#endif
