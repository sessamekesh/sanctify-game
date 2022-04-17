#ifndef SANCTIFY_COMMON_LOGIC_NETSYNC_NETSYNC_H
#define SANCTIFY_COMMON_LOGIC_NETSYNC_NETSYNC_H

#include <cstdint>

namespace sanctify::logic {

const int kSanctifyMagicHeader = 0x00DADB0D;

struct NetSyncId {
  uint32_t id;

  bool operator==(const NetSyncId&) const;
  bool operator<(const NetSyncId&) const;
};

}  // namespace sanctify::logic

#endif
