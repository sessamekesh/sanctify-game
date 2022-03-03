#include <ecs/components/parent_entity_component.h>
#include <ecs/components/solid_animated_components.h>
#include <ecs/systems/attach_player_renderables.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/gameplay/player_definition_components.h>
#include <util/logical_to_render_utils.h>

using namespace sanctify;
using namespace ecs;

namespace {
entt::entity create_renderable_entity(
    entt::registry& world, entt::entity parent,
    ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key geo_key,
    ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key
        material_key) {
  auto e = world.create();

  world.emplace<ParentEntityComponent>(e, parent);
  world.emplace<SolidAnimatedRenderableComponent>(e, geo_key, material_key,
                                                  glm::mat4(1.f));

  return e;
}
}  // namespace

AttachPlayerRenderablesSystem::AttachPlayerRenderablesSystem(
    ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key
        joint_material_key,
    ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key
        base_material_key,
    ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key
        joint_geo_key,
    ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key
        base_geo_key)
    : joint_material_key_(joint_material_key),
      base_material_key_(base_material_key),
      joint_geo_key_(joint_geo_key),
      base_geo_key_(base_geo_key) {}

void AttachPlayerRenderablesSystem::run(entt::registry& world) const {
  auto view = world.view<const component::BasicPlayerComponent,
                         const component::MapLocation,
                         const component::OrientationComponent>();

  for (auto [e, map_location, orientation] : view.each()) {
    if (!world.all_of<PlayerRenderablesChildrenComponent>(e)) {
      auto joint_entity = ::create_renderable_entity(world, e, joint_geo_key_,
                                                     joint_material_key_);
      auto base_entity = ::create_renderable_entity(world, e, base_geo_key_,
                                                    base_material_key_);

      world.emplace<PlayerRenderablesChildrenComponent>(e, joint_entity,
                                                        base_entity);
    }

    auto& children = world.get<PlayerRenderablesChildrenComponent>(e);

    // TODO (sessamekesh): What should the scale actually be?
    const glm::vec3 kScl = glm::vec3(0.06f, 0.06f, 0.06f);
    glm::mat4 mat_world = ltr_utils::get_transform(
        map_location.XZ, orientation.orientation, kScl);

    world.get<SolidAnimatedRenderableComponent>(children.baseRenderableEntity)
        .matWorld = mat_world;
    world.get<SolidAnimatedRenderableComponent>(children.jointRenderableEntity)
        .matWorld = mat_world;
  }
}
