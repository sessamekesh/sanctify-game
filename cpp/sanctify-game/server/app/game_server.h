#ifndef SANCTIFY_GAME_SERVER_APP_GAME_SERVER_H
#define SANCTIFY_GAME_SERVER_APP_GAME_SERVER_H

#include <app/net_events.h>
#include <igasync/promise.h>
#include <sanctify-game-common/gameplay/standard_target_travel_system.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/concurrentqueue.h>
#include <util/types.h>
#include <util/visit.h>

#include <entt/entt.hpp>
#include <map>
#include <mutex>

namespace sanctify {

/**
 * Game app implementation - maintains actual game state and runs game logic.
 *
 * Exposes a "Player" interface
 */
class GameServer : public std::enable_shared_from_this<GameServer> {
 public:
  using PlayerMessageCallback =
      std::function<void(PlayerId, sanctify::pb::GameServerMessage)>;

 public:
  // Initialization
  GameServer();
  ~GameServer();

  using GameInitializePromise =
      indigo::core::Promise<indigo::core::EmptyPromiseRsl>;
  std::shared_ptr<GameInitializePromise> start_game();

  // Outside interface
  void receive_message_for_player(PlayerId player_id,
                                  sanctify::pb::GameClientMessage client_msg);
  void set_player_message_receiver(PlayerMessageCallback cb);

  std::shared_ptr<indigo::core::Promise<bool>> try_connect_player(
      const PlayerId& player_id);
  void notify_player_dropped_connection(const PlayerId& player_id);

  std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
  shutdown();

 private:
  // Internal helpers...
  void process_net_events();
  void handle_net_event(NetEvent& event);
  void handle_connect_player_event(ConnectPlayerEvent& evt);
  void handle_player_disconnection(DisconnectPlayerEvent& evt);

  void apply_player_inputs();
  void apply_single_player_input(const PlayerId& player_id,
                                 const pb::GameClientMessage& message);

  void update(float dt);
  void send_player_updates();

  // Inidivual handlers for net client inputs...
  void handle_travel_to_location_request(
      entt::entity player_entity, const pb::TravelToLocationRequest& request);

 private:
  PlayerMessageCallback player_message_cb_;
  std::thread game_thread_;
  bool is_running_;  // Not guarded - set once to end game, no races (careful!)
  float sim_clock_;

  entt::registry world_;
  system::StandardTargetTravelSystem standard_target_travel_system_;
  std::unordered_map<PlayerId, entt::entity, PlayerIdHashFn> player_entities_;

  moodycamel::ConcurrentQueue<std::pair<PlayerId, pb::GameClientMessage>>
      message_queue_;

  moodycamel::ConcurrentQueue<NetEvent> net_events_queue_;
};

}  // namespace sanctify

#endif
