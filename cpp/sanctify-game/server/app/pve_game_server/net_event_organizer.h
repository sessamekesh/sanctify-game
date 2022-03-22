#ifndef SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_NET_EVENT_ORGANIZER_H
#define SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_NET_EVENT_ORGANIZER_H

/**
 * Class that offers thread safety around receiving network events, parsing
 *  them, and responding with appropriate events to the client.
 *
 * Use case:
 * - Number of players is known ahead of time, so arrays can be set up ahead
 *   of time to allow individual player locking (instead of entire map locks)
 * - Side threads can send events to this at any time, from multiple connected
 *   players. Locking for one player should not affect locking for another.
 */

#include <igcore/maybe.h>
#include <net/net_server.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

#include <glm/glm.hpp>
#include <shared_mutex>
#include <vector>

namespace sanctify {

class NetEventOrganizer {
 public:
  struct PingRecord {
    indigo::core::Maybe<uint32_t> pingId;
    float pingData;
  };

 public:
  NetEventOrganizer(uint32_t num_players);

  bool add_player(const PlayerId& player_id);
  bool remove_player(const PlayerId& player_id);
  void recv_message(const PlayerId& player_id, pb::GameClientMessage msg);

  indigo::core::Maybe<glm::vec2> get_move_command(const PlayerId& pid);
  indigo::core::Maybe<uint32_t> last_acked_snapshot_id(const PlayerId& pid);
  bool ready_state(const PlayerId& pid) const;
  indigo::core::Maybe<PingRecord> ping_record(const PlayerId& pid);

  NetServer::PlayerConnectionState net_server_state(const PlayerId& pid);
  void set_net_server_state(const PlayerId& pid,
                            NetServer::PlayerConnectionState state);

 private:
  indigo::core::Maybe<uint32_t> row_idx(const PlayerId& player_id) const;
  void travel_to_location_request_event(const PlayerId& player_id,
                                        const pb::PlayerMovement& msg);
  void snapshot_received_event(const PlayerId& player_id,
                               const pb::SnapshotReceived& msg);
  void handle_ping(const PlayerId& player_id, const pb::ClientPing& msg);

 private:
  uint32_t num_players_;

  mutable std::vector<std::shared_mutex> m_player_id_;
  std::vector<indigo::core::Maybe<PlayerId>> player_ids_;

  std::vector<std::mutex> m_move_commands_;
  std::vector<indigo::core::Maybe<glm::vec2>> move_commands_;

  std::vector<std::mutex> m_last_acked_snapshots_;
  std::vector<indigo::core::Maybe<uint32_t>> last_acked_snapshots_;

  std::vector<std::mutex> m_netserver_state_;
  std::vector<NetServer::PlayerConnectionState> netserver_states_;

  mutable std::vector<std::mutex> m_ready_state_;
  std::vector<bool> ready_states_;

  mutable std::vector<std::mutex> m_ping_record_;
  std::vector<indigo::core::Maybe<PingRecord>> ping_records_;
};

}  // namespace sanctify

#endif
