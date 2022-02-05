#include <game_scene/systems/player_render_system.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>

using namespace sanctify;
using namespace system;
using namespace indigo;
using namespace core;

PodVector<solid_animated::MatWorldInstanceData>
PlayerRenderSystem::get_instance_data(entt::registry& world) {
  PodVector<solid_animated::MatWorldInstanceData> data(4);

  // TODO (sessamekesh): this rotation will be a common case for Assimp files,
  //  you should handle this in pre-processing.

  // TODO (sessamekesh): a separate system should be in charge of updating
  //  the scene graph, and you should instead hook into scene nodes.

  glm::mat4 make_tiny =
      glm::scale(glm::mat4(1.f), glm::vec3(0.06f, 0.06f, 0.06f));

  auto player_pos_view = world.view<const component::MapLocation>();

  for (auto& [entity, map_location] : player_pos_view.each()) {
    glm::mat4 move_to_location = glm::translate(
        glm::mat4(1.f), glm::vec3(map_location.XZ.x, 0.f, map_location.XZ.y));

    data.push_back({move_to_location * make_tiny});
  }

  return data;
}