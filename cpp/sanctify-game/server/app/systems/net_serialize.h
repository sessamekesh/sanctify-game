#ifndef SANCTIFY_GAME_SERVER_APP_SYSTEMS_NET_SERIALIZE_H
#define SANCTIFY_GAME_SERVER_APP_SYSTEMS_NET_SERIALIZE_H

/**
 * Net serialization system - computes a snapshot state for a given player.
 *
 * Also keeps track of previous snapshots that the player has seen, and sends
 *  only an appropriate diff.
 */

#include <igcore/maybe.h>
#include <sanctify-game-common/net/game_snapshot.h>
#include <util/types.h>

#include <map>

namespace sanctify::system {

class NetSerializeSystem {
 public:
  NetSerializeSystem(float time_between_diffs, float time_between_snapshots);

  void receive_client_snapshot_ack(const PlayerId& player_id,
                                   uint32_t snapshot_id);

  indigo::core::Maybe<GameSnapshot> get_snapshot_to_send(
      const PlayerId& player_id, entt::registry& world,
      entt::entity player_entity, float server_time);

  indigo::core::Maybe<GameSnapshotDiff> get_diff_to_send(
      const PlayerId& player_id, entt::registry& world,
      entt::entity player_entity, float server_time);

 private:
  float next_snapshot_time(const PlayerId& player_id) const;
  void set_next_snapshot_time(const PlayerId& player_id, float next_time);

  float next_diff_time(const PlayerId& player_id) const;
  void set_next_diff_time(const PlayerId& player_id, float next_time);

  void set_snapshot(const PlayerId& player_id, GameSnapshot snapshot);

  uint32_t get_and_inc_snapshot_id(const PlayerId& player_id);

  GameSnapshot gen_snapshot_for_player(float server_time,
                                       const PlayerId& player_id,
                                       entt::entity player_entity,
                                       entt::registry& world);
  indigo::core::Maybe<GameSnapshot> get_base_snapshot_for_player(
      const PlayerId& player_id);

  float time_between_full_snapshots_;
  float time_between_diffs_;

  std::map<PlayerId, float> time_of_next_full_snapshot_;
  std::map<PlayerId, float> time_of_next_diff_;
  std::map<PlayerId, uint32_t> next_snapshot_id_;
  std::set<PlayerId> ready_players_;

  std::map<PlayerId, std::map<uint32_t, GameSnapshot>> sent_snapshots_;
};

}  // namespace sanctify::system

#endif
