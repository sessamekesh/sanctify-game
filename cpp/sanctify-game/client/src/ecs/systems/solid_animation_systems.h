#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_SYSTEMS_SOLID_ANIMATION_SYSTEMS_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_SYSTEMS_SOLID_ANIMATION_SYSTEMS_H

#include <ecs/components/solid_animated_components.h>

#include <entt/entt.hpp>

namespace sanctify::ecs {

class SetOzzAnimationKeysSystem {
 public:
  SetOzzAnimationKeysSystem(
      ReadonlyResourceRegistry<ozz::animation::Skeleton>::Key ybot_skeleton_key,
      ReadonlyResourceRegistry<ozz::animation::Animation>::Key ybot_idle_key,
      ReadonlyResourceRegistry<ozz::animation::Animation>::Key ybot_walk_key);

 public:
  void update(entt::registry& world, float dt) const;

 private:
  ReadonlyResourceRegistry<ozz::animation::Skeleton>::Key ybot_skeleton_key_;
  ReadonlyResourceRegistry<ozz::animation::Animation>::Key ybot_idle_key_;
  ReadonlyResourceRegistry<ozz::animation::Animation>::Key ybot_walk_key_;
};

class UpdateOzzAnimationBuffersSystem {
 public:
  UpdateOzzAnimationBuffersSystem(
      std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Skeleton>>
          skeleton_registry,
      std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Animation>>
          animation_registry,
      std::shared_ptr<
          ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>>
          solid_animated_geo_registry);

  void update_cpu_buffers(entt::registry& world) const;
  void update_gpu_buffers(entt::registry& world,
                          const solid_animated::SolidAnimatedPipeline& pipeline,
                          const wgpu::Device& device) const;

 private:
  std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Skeleton>>
      skeleton_registry_;
  std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Animation>>
      animation_registry_;
  std::shared_ptr<ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>>
      solid_animated_geo_registry_;
};

class RenderSolidRenderablesSystem {
 public:
  RenderSolidRenderablesSystem(
      std::shared_ptr<
          ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>>
          solid_animated_geo_registry,
      std::shared_ptr<
          ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>>
          solid_animated_material_registry);

  void render(entt::registry& world, const wgpu::Device& device,
              const wgpu::RenderPassEncoder& pass,
              const solid_animated::SolidAnimatedPipeline& pipeline,
              const solid_animated::ScenePipelineInputs& scene_inputs,
              const solid_animated::FramePipelineInputs& frame_inputs);

 private:
  std::shared_ptr<ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>>
      solid_animated_geo_registry_;
  std::shared_ptr<
      ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>>
      solid_animated_material_registry_;
};

}  // namespace sanctify::ecs

#endif
