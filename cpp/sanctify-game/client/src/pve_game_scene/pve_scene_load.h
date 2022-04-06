#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_PVE_SCENE_LOAD_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_PVE_SCENE_LOAD_H

#include <app_base.h>
#include <igasync/promise.h>

#include <entt/entt.hpp>

namespace sanctify::pve {

/**
 * Load all resources required to run PVE game scene simulation into world
 *  object. Promise resolves as "true" in success case, "false" otherwise.
 *
 * TODO (sessamekesh): maybe return triggers for individual parts for tracking
 *  load progress, like when certain shaders are loaded?
 */
std::shared_ptr<indigo::core::Promise<bool>> load_pve_scene(
    entt::registry& world,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    std::shared_ptr<AppBase> app_base);

}  // namespace sanctify::pve

#endif
