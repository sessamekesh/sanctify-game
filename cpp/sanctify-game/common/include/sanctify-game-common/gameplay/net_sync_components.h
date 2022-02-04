#ifndef SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_NET_SYNC_COMPONENTS_H
#define SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_NET_SYNC_COMPONENTS_H

#include <cstdint>

namespace sanctify::component {

struct NetSyncId {
  uint32_t Id;

  bool operator==(const NetSyncId& o) const;
  bool operator<(const NetSyncId& o) const;
};

}  // namespace sanctify::component

#endif
