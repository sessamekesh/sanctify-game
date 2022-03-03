#include <ecs/components/parent_entity_component.h>
#include <ecs/systems/destroy_children_system.h>

using namespace sanctify;
using namespace ecs;

void DestroyChildrenSystem::run(entt::registry& world) {
  bool did_something = false;

  do {
    auto view = world.view<ParentEntityComponent>();
    for (auto [e, parent] : view.each()) {
      if (!world.valid(parent.parentEntity)) {
        world.destroy(e);
        did_something = true;
      }
    }
  } while (did_something);
}
