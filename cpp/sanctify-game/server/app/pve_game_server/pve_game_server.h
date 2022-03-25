#ifndef SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_PVE_GAME_SERVER_H
#define SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_PVE_GAME_SERVER_H

/**
 * Game server implementation for the Sanctify PvE game.
 *
 * Can accept network connections immediately after being created, but will not
 *  necessarily start the game until the server has all resources loaded, and
 *  all clients are connected and ready to go
 */

#include <app/ecs/player_nav_system.h>
#include <app/pve_game_server/ecs/net_state_update_system.h>
#include <app/pve_game_server/ecs/send_client_messages_system.h>
#include <app/pve_game_server/net_event_organizer.h>
#include <app/systems/locomotion.h>
#include <igcore/bimap.h>
#include <ignav/detour_navmesh.h>
#include <net/net_server.h>
#include <sanctify-game-common/gameplay/locomotion.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

#include <entt/entt.hpp>
#include <memory>
#include <vector>

namespace sanctify {

class PveGameServer : public std::enable_shared_from_this<PveGameServer> {
 public:
  using PlayerMessageCb = std::function<void(PlayerId, pb::GameServerMessage)>;

  /** Determines how game ticks and messages are handled */
  enum class ServerStage {
    /** Game server is initializing, and is not ready to consider players */
    Initializing,

    /** Server is waiting on clients to be ready */
    WaitingForPlayers,

    /** Game simulation is running in earnest */
    Running,

    /** A win condition has been met, and the server preparing to shut down */
    GameOver,

    /** The simulation has finished and the server should be shut down */
    Terminated,
  };

 public:
  static std::shared_ptr<PveGameServer> Create(
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      bool use_fixed_timestamp);

  // Netcomms (outside interface)
  void receive_message_for_player(PlayerId player_id,
                                  sanctify::pb::GameClientMessage client_msg);
  void set_player_message_receiver(PlayerMessageCb cb);
  void set_netstate(const PlayerId& player_id,
                    NetServer::PlayerConnectionState connection_state);

  std::shared_ptr<indigo::core::Promise<bool>> try_connect_player(
      const PlayerId& player_id);

  std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
  shutdown();

 private:
  PveGameServer(std::shared_ptr<indigo::core::TaskList> async_task_list,
                bool use_fixed_timestamp);
  void initialize();

  void update(float dt);
  void update_waiting_for_players();
  void update_running();
  void update_game_over();

  void maybe_queue_pong(const PlayerId& pid, entt::entity e);

  void create_initial_game_scene();

 private:
  // Bookkeeping
  bool use_fixed_timestamp_;
  ServerStage server_stage_;
  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;
  std::shared_ptr<indigo::core::TaskList> async_task_list_;
  std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
      player_cb_set_;
  std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
      shutdown_promise_;

  // Netcode stuff
  PlayerMessageCb player_message_cb_;
  NetEventOrganizer net_event_organizer_;
  uint32_t next_net_sync_id_;

  // Simulation internals
  std::thread game_thread_;
  bool is_running_;  // Not guarded - set once to end game, no races (careful!)
  entt::registry world_;

  // Systems
  ecs::NetStateUpdateSystem net_state_update_system_;
  system::PlayerNavSystem player_nav_system_;
  system::LocomotionSystem locomotion_system_;
  ecs::QueueClientMessagesSystem queue_client_messages_system_;

  // Game server resources (logic helpers)
  indigo::core::Maybe<indigo::nav::DetourNavmesh> navmesh_;
};

}  // namespace sanctify

#endif
