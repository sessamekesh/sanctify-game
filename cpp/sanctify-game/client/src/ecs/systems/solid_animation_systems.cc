#include <ecs/components/parent_entity_component.h>
#include <ecs/components/solid_animated_components.h>
#include <ecs/systems/solid_animation_systems.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <render/solid_animated/solid_animated_pipeline.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/gameplay/player_definition_components.h>

using namespace sanctify;
using namespace ecs;

SetOzzAnimationKeysSystem::SetOzzAnimationKeysSystem(
    ReadonlyResourceRegistry<ozz::animation::Skeleton>::Key ybot_skeleton_key,
    ReadonlyResourceRegistry<ozz::animation::Animation>::Key ybot_idle_key,
    ReadonlyResourceRegistry<ozz::animation::Animation>::Key ybot_walk_key)
    : ybot_skeleton_key_(ybot_skeleton_key),
      ybot_idle_key_(ybot_idle_key),
      ybot_walk_key_(ybot_walk_key) {}

void SetOzzAnimationKeysSystem::update(entt::registry& world, float dt) const {
  auto view = world.view<const component::MapLocation,
                         const component::BasicPlayerComponent>();

  for (auto&& [e, map_location] : view.each()) {
    bool has_nav = world.all_of<component::NavWaypointList>(e);

    auto& animation_state_component =
        world.get_or_emplace<OzzAnimationStateComponent>(
            e, ybot_skeleton_key_, ybot_idle_key_, 0.f, 1.f);

    if (has_nav) {
      if (animation_state_component.animationKey == ybot_idle_key_) {
        animation_state_component.animationTime = 0.f;
      } else {
        animation_state_component.animationTime += dt;
      }

      animation_state_component.animationKey = ybot_walk_key_;
    } else {
      if (animation_state_component.animationKey == ybot_walk_key_) {
        animation_state_component.animationTime = 0.f;
      } else {
        animation_state_component.animationTime += dt;
      }

      animation_state_component.animationKey = ybot_idle_key_;
    }
  }
}

UpdateOzzAnimationBuffersSystem::UpdateOzzAnimationBuffersSystem(
    std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Skeleton>>
        skeleton_registry,
    std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Animation>>
        animation_registry,
    std::shared_ptr<ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>>
        solid_animated_geo_registry)
    : skeleton_registry_(skeleton_registry),
      animation_registry_(animation_registry),
      solid_animated_geo_registry_(solid_animated_geo_registry) {}

void UpdateOzzAnimationBuffersSystem::update_cpu_buffers(
    entt::registry& world) const {
  auto view = world.view<const OzzAnimationStateComponent>();

  for (auto&& [e, animation_state] : view.each()) {
    // TODO (sessamekesh): Early out if this entity is not visible in the scene

    OzzAnimationCpuBuffersComponent& buffers =
        world.get_or_emplace<OzzAnimationCpuBuffersComponent>(
            e, (uint32_t)solid_animated::SolidAnimatedPipeline::kMaxBonesCount);

    auto* skeleton = skeleton_registry_->get(animation_state.skeletonKey);
    auto* animation = animation_registry_->get(animation_state.animationKey);

    if (!skeleton || !animation) {
      // TODO (sessamekesh): propagate errors to an accumulator, or return from
      // this method (also do this for job run methods below)
      continue;
    }

    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = animation;
    sampling_job.context = buffers.context.get();
    sampling_job.ratio =
        (animation_state.animationTime * animation_state.animationTimeRatio) /
        animation->duration();
    sampling_job.output =
        ozz::span(buffers.locals.raw(), buffers.locals.size());
    if (!sampling_job.Run()) {
      continue;
    }

    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = skeleton;
    ltm_job.input = ozz::span(buffers.locals.raw(), buffers.locals.size());
    ltm_job.output = ozz::span(buffers.models.raw(), buffers.models.size());
    if (!ltm_job.Run()) {
      continue;
    }
  }
}

void UpdateOzzAnimationBuffersSystem::update_gpu_buffers(
    entt::registry& world,
    const solid_animated::SolidAnimatedPipeline& pipeline,
    const wgpu::Device& device) const {
  auto view = world.view<const ParentEntityComponent,
                         const SolidAnimatedRenderableComponent>();

  for (auto [e, parent_entity, solid_animated_renderable] : view.each()) {
    // TODO (sessamekesh): Early out if this entity is not visible in the scene

    if (!world.valid(parent_entity.parentEntity)) {
      continue;
    }

    auto* anim_buffers = world.try_get<OzzAnimationCpuBuffersComponent>(
        parent_entity.parentEntity);
    auto* anim_keys =
        world.try_get<OzzAnimationStateComponent>(parent_entity.parentEntity);

    if (!anim_buffers || !anim_keys) {
      continue;
    }

    auto* skeleton = skeleton_registry_->get(anim_keys->skeletonKey);
    auto* geo =
        solid_animated_geo_registry_->get(solid_animated_renderable.geoKey);

    if (!skeleton || !geo) {
      continue;
    }

    OzzAnimationGpuBuffersComponent& gpu_buffers =
        world.get_or_emplace<OzzAnimationGpuBuffersComponent>(
            e, device, pipeline,
            solid_animated::SolidAnimatedPipeline::kMaxBonesCount);

    for (int i = 0; i < skeleton->num_joints(); i++) {
      const auto& ozz_model = anim_buffers->models[i];
      // TODO (sessamekesh): More elegantly extract values to GLM,
      // or just construct and do math in OZZ space
      glm::mat4 model(((float*)&ozz_model)[0], ((float*)&ozz_model)[1],
                      ((float*)&ozz_model)[2], ((float*)&ozz_model)[3],
                      ((float*)&ozz_model)[4], ((float*)&ozz_model)[5],
                      ((float*)&ozz_model)[6], ((float*)&ozz_model)[7],
                      ((float*)&ozz_model)[8], ((float*)&ozz_model)[9],
                      ((float*)&ozz_model)[10], ((float*)&ozz_model)[11],
                      ((float*)&ozz_model)[12], ((float*)&ozz_model)[13],
                      ((float*)&ozz_model)[14], ((float*)&ozz_model)[15]);
      glm::mat4 inv_bind = geo->invBindPoses[i];

      gpu_buffers.shaderValues[i] = model * inv_bind;
    }

    gpu_buffers.animationPipelineInputs.SkinMatricesUbo.update(
        device, gpu_buffers.shaderValues);
  }
}

RenderSolidRenderablesSystem::RenderSolidRenderablesSystem(
    std::shared_ptr<ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>>
        solid_animated_geo_registry,
    std::shared_ptr<
        ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>>
        solid_animated_material_registry)
    : solid_animated_geo_registry_(solid_animated_geo_registry),
      solid_animated_material_registry_(solid_animated_material_registry) {}

void RenderSolidRenderablesSystem::render(
    entt::registry& world, const wgpu::Device& device,
    const wgpu::RenderPassEncoder& pass,
    const solid_animated::SolidAnimatedPipeline& pipeline,
    const solid_animated::ScenePipelineInputs& scene_inputs,
    const solid_animated::FramePipelineInputs& frame_inputs) {
  auto view = world.view<const SolidAnimatedRenderableComponent,
                         OzzAnimationGpuBuffersComponent>();

  if (view.size_hint() == 0u) {
    return;
  }

  solid_animated::RenderUtil render_util(pass, pipeline);
  render_util.set_scene_inputs(scene_inputs).set_frame_inputs(frame_inputs);

  indigo::core::PodVector<solid_animated::MatWorldInstanceData> instances(4);

  for (auto [e, solid_animated_renderable, gpu_buffers] : view.each()) {
    const solid_animated::MaterialPipelineInputs* mat_inputs =
        solid_animated_material_registry_->get(
            solid_animated_renderable.materialKey);
    auto* geo =
        solid_animated_geo_registry_->get(solid_animated_renderable.geoKey);

    if (!mat_inputs || !geo) {
      continue;
    }

    auto& instance_buffer =
        world.get_or_emplace<SolidAnimatedRenderableInstanceBufferComponent>(
            e, device);

    instances.resize(0);
    instances.push_back({solid_animated_renderable.matWorld});
    instance_buffer.buffer.update_index_data(device, instances);

    render_util.set_material_inputs(*mat_inputs)
        .set_instances(instance_buffer.buffer)
        .set_animation_inputs(gpu_buffers.animationPipelineInputs)
        .set_geometry(*geo)
        .draw();
  }
}
