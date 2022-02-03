#ifndef SANCTIFY_GAME_SERVER_APP_NET_COMPONENTS_H
#define SANCTIFY_GAME_SERVER_APP_NET_COMPONENTS_H

#include <util/types.h>

namespace sanctify::component {

struct ClientSyncMetadata {
  PlayerId playerId;
  float LastUpdateSentTime;
};

}  // namespace sanctify::component

#endif
