#ifndef SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_RENDER_CLIENT_SCHEDULER_H
#define SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_RENDER_CLIENT_SCHEDULER_H

#include <igecs/scheduler.h>

namespace sanctify::pve {

indigo::igecs::Scheduler build_render_client_scheduler();

}

#endif
