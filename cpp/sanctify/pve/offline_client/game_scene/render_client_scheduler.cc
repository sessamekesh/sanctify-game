#include "render_client_scheduler.h"

#include <common/render/viewport/update_arena_camera_system.h>

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

  return builder.build();
}
