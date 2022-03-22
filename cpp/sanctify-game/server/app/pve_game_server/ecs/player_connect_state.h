#ifndef SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_PLAYER_CONNECT_STATE_H
#define SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_PLAYER_CONNECT_STATE_H

#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

namespace sanctify {

enum class PlayerConnectState {
  /** This player has either never been connected, or is considered dropped */
  Disconnected,

  /** This player is connected and healty */
  Healthy,

  /**
   * This player is connected, but messages have not been acked in a while, so
   *  back off on communication until it becomes healthy again
   */
  Unhealthy,
};

enum class PlayerConnectionType {
  None,
  WebSocket,
  WebRTC,
};

enum class PlayerReadyState {
  NotReady,
  Ready,
};

struct PlayerConnectStateComponent {
  PlayerId playerId;
  PlayerConnectState connectState;
  PlayerConnectionType connectionType;
  PlayerReadyState readyState;
};

struct PlayerNetStateComponent {
  float lastMessageReceivedTime;
  float lastSnapshotSentTime;
  float lastDiffSentTime;

  uint32_t lastAckedSnapshotId;
};

struct QueuedMessagesComponent {
  std::vector<pb::GameServerSingleMessage> unsentMessages;
};

}  // namespace sanctify

#endif
