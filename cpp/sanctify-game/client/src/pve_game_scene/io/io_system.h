#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_IO_IO_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_IO_IO_SYSTEM_H

#include <entt/entt.hpp>

namespace sanctify::pve {

class IoSystem {
 public:
  // Update I/O state
  static void update_io(entt::registry& world);
};

}  // namespace sanctify::pve

#endif
