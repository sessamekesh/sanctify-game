#ifndef SANCTIFY_GAME_SERVER_NET_TYPES_H
#define SANCTIFY_GAME_SERVER_NET_TYPES_H

#include <cstdint>

namespace sanctify {

struct PlayerId {
  std::uint64_t Id;

  bool operator<(const PlayerId& o) const { return Id < o.Id; }
};

struct GameId {
  std::uint64_t Id;

  bool operator()(const sanctify::GameId& lhs,
                  const sanctify::GameId& rhs) const {
    return lhs.Id < rhs.Id;
  }
};

}  // namespace sanctify

#endif
