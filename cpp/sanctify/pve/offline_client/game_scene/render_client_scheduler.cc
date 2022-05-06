#include "render_client_scheduler.h"

#include <common/render/common/render_components.h>
#include <common/render/solid_static/ecs_util.h>
#include <common/render/viewport/update_arena_camera_system.h>

#include "pve_offline_render.h"

using namespace sanctify;

using namespace indigo;
using namespace core;

igecs::Scheduler pve::build_render_client_scheduler() {
  auto builder = igecs::Scheduler::Builder();
  builder.max_spin_time(std::chrono::milliseconds(50));

  auto update_common_ubos =
      builder.add_node()
          .main_thread_only()
          .with_decl(render::UpdateArenaCameraSystem::update_decl())
          .build([](igecs::WorldView* wv) {
            render::UpdateArenaCameraSystem::update(wv);
            return Promise<EmptyPromiseRsl>::immediate({});
          });

  auto render_scene = builder.add_node()
                          .main_thread_only()
                          .with_decl(PveOfflineRenderSystem::render_decl())
                          .depends_on(update_common_ubos)
                          .build([](igecs::WorldView* wv) {
                            PveOfflineRenderSystem::render(wv);
                            return Promise<EmptyPromiseRsl>::immediate({});
                          });

  // TODO (sessamekesh): Make a system for actually rendering.
  //  Maybe record commands on separate threads? Ooooh fancy
  //  Otherwise just record to the command buffer and move along.

  return builder.build();
}
