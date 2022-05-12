#include "update_client_scheduler.h"

#include "systems/process_user_input.h"
#include "systems/stage_client_message.h"

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

indigo::igecs::Scheduler UpdateClientScheduler::build() {
  auto builder = igecs::Scheduler::Builder();
  builder.max_spin_time(std::chrono::milliseconds(20));

  auto process_user_input = builder.add_node()
                                .with_decl(ProcessUserInputSystem::decl())
                                .build(ProcessUserInputSystem::update);

  auto stage_client_messages = builder.add_node()
                                   .with_decl(StageClientMessageSystem::decl())
                                   .depends_on(process_user_input)
                                   .build(StageClientMessageSystem::run);

  return builder.build();
}
