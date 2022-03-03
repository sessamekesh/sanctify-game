#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_SYSTEMS_ATTACH_PLAYER_RENDERABLES_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_SYSTEMS_ATTACH_PLAYER_RENDERABLES_H

#include <render/solid_animated/solid_animated_geo.h>
#include <render/solid_animated/solid_animated_pipeline.h>
#include <util/resource_registry.h>

#include <entt/entt.hpp>

namespace sanctify::ecs {

struct PlayerRenderablesChildrenComponent {
  entt::entity jointRenderableEntity;
  entt::entity baseRenderableEntity;
};

class AttachPlayerRenderablesSystem {
 public:
  AttachPlayerRenderablesSystem(
      ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key
          joint_material_key,
      ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key
          base_material_key,
      ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key
          joint_geo_key,
      ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key
          base_geo_key);

  void run(entt::registry& world) const;

 private:
  ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key
      joint_material_key_;
  ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key
      base_material_key_;

  ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key
      joint_geo_key_;
  ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key base_geo_key_;
};

}  // namespace sanctify::ecs

#endif
