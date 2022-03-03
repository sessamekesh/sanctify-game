#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_COMPONENTS_SOLID_ANIMATED_COMPONENTS_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_COMPONENTS_SOLID_ANIMATED_COMPONENTS_H

#include <igcore/pod_vector.h>
#include <iggpu/thin_ubo.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/maths/soa_float4x4.h>
#include <ozz/base/maths/soa_transform.h>
#include <render/solid_animated/solid_animated_geo.h>
#include <render/solid_animated/solid_animated_pipeline.h>
#include <util/resource_registry.h>

#include <glm/glm.hpp>

/**
 * Okay, so this is something I'm not sure I like - there's one animation that
 *  controls multiple meshes, so the OZZ animation components exist on a parent
 *  entity while actual Renderable and GPUBuffer stuff exists on children.
 * I _think_ this will work long term, but it's a sort of wonky thing.
 *
 * Parent components:
 * - OzzAnimationStateComponent (generated)
 * - OzzAnimationCpuBuffersComponent (generated)
 *
 * Child components:
 * - SolidAnimatedRenderableComponent
 * - SolidAnimatedRenderableInstanceBufferComponent (generated)
 * - OzzAnimationGpuBuffersComponent (generated)
 */

namespace sanctify::ecs {

struct SolidAnimatedRenderableComponent {
  ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key geoKey;
  ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key
      materialKey;
  glm::mat4 matWorld;
};

// TODO (sessamekesh): Solid animated renderables should not be animated, eh?
struct SolidAnimatedRenderableInstanceBufferComponent {
  SolidAnimatedRenderableInstanceBufferComponent(const wgpu::Device& device)
      : buffer(device) {}

  solid_animated::MatWorldInstanceBuffer buffer;
};

struct OzzAnimationStateComponent {
  ReadonlyResourceRegistry<ozz::animation::Skeleton>::Key skeletonKey;
  ReadonlyResourceRegistry<ozz::animation::Animation>::Key animationKey;

  float animationTime;
  float animationTimeRatio;
};

struct OzzAnimationCpuBuffersComponent {
  OzzAnimationCpuBuffersComponent(uint32_t buffer_size)
      : context(std::make_unique<ozz::animation::SamplingJob::Context>(
            buffer_size)),
        locals(buffer_size),
        models(buffer_size) {
    locals.resize(buffer_size);
    models.resize(buffer_size);
  }

  std::unique_ptr<ozz::animation::SamplingJob::Context> context;
  indigo::core::PodVector<ozz::math::SoaTransform> locals;
  indigo::core::PodVector<ozz::math::Float4x4> models;
};

struct OzzAnimationGpuBuffersComponent {
  OzzAnimationGpuBuffersComponent(
      const wgpu::Device& device,
      const solid_animated::SolidAnimatedPipeline& pipeline,
      uint32_t buffer_size)
      : shaderValues(buffer_size),
        animationPipelineInputs(pipeline.create_animation_inputs(device)) {
    shaderValues.resize(buffer_size);
  }

  OzzAnimationGpuBuffersComponent(OzzAnimationGpuBuffersComponent&&) = default;
  OzzAnimationGpuBuffersComponent& operator=(
      OzzAnimationGpuBuffersComponent&&) = default;

  indigo::core::PodVector<glm::mat4x4> shaderValues;
  solid_animated::AnimationPipelineInputs animationPipelineInputs;
};

}  // namespace sanctify::ecs

#endif
