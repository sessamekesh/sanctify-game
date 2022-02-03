#ifndef SANCTIFY_GAME_SERVER_UTIL_TYPES_H
#define SANCTIFY_GAME_SERVER_UTIL_TYPES_H

#include <cstdint>

namespace sanctify {

struct PlayerId {
  std::uint64_t Id;

  bool operator<(const PlayerId& o) const { return Id < o.Id; }

  bool operator==(const PlayerId& o) const { return Id == o.Id; }
};

struct PlayerIdHashFn {
  size_t operator()(const PlayerId& id) const {
    return std::hash<uint64_t>()(id.Id);
  }
};

struct GameId {
  std::uint64_t Id;

  // Janky hack to allow using this as an ordered map key
  bool operator()(const sanctify::GameId& lhs,
                  const sanctify::GameId& rhs) const {
    return lhs.Id < rhs.Id;
  }
};

}  // namespace sanctify

#endif