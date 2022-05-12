#ifndef SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_UPDATE_CLIENT_SCHEDULER_H
#define SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_UPDATE_CLIENT_SCHEDULER_H

#include <igecs/scheduler.h>

namespace sanctify::pve {

class UpdateClientScheduler {
 public:
  static indigo::igecs::Scheduler build();
};

}  // namespace sanctify::pve

#endif
