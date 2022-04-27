#ifndef SANCTIFY_COMMON_RENDER_SOLID_STATIC_ECS_UTIL_H
#define SANCTIFY_COMMON_RENDER_SOLID_STATIC_ECS_UTIL_H

#include <common/render/common/pipeline_build_error.h>
#include <common/util/resource_registry.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise.h>
#include <igcore/maybe.h>
#include <igecs/world_view.h>

#include "instance_store.h"
#include "pipeline.h"
#include "solid_static_geo.h"

/**
 * ECS context components, entity components, and utility methods for dealing
 *  with solid static renderables in a game world
 */

namespace sanctify::render::solid_static {

/** SolidStatic specific object inputs (geometry type) */
struct RenderableComponent {
  ReadonlyResourceRegistry<Geo>::Key geoKey;
  glm::vec3 albedo;
  float metallic;
  float roughness;
  float ao;
};

/** Top-level registries for geo and instance buffers */
struct CtxResourceRegistries {
  ReadonlyResourceRegistry<Geo> geoRegistry;
  InstanceStore instanceStore;
};

/** Pipeline (asynchronously created) */
struct CtxPipeline {
  PipelineBuilder builder;
  Pipeline currentPipeline;
};

/** Utility methods for dealing with SolidStatic rendering */
struct EcsUtil {
  // Setup state required to deal with SolidStatic rendering. Returns true
  //  if successful (requires CtxHdrFramebufferParams). Notice - this does not
  //  attach the pipeline, which must be done asynchronously!
  static bool prepare_world(indigo::igecs::WorldView* wv);

  // Individual renderables
  static void attach_renderable(indigo::igecs::WorldView* wv, entt::entity e,
                                ReadonlyResourceRegistry<Geo>::Key geo_key,
                                InstanceData instance_data);
  static void detach_renderable(indigo::igecs::WorldView* wv, entt::entity e);

  // Pipeline creation and fetching
  // Notice: these require CtxHdrFramebufferParams to have been set
  static std::shared_ptr<
      indigo::core::Promise<indigo::core::Maybe<PipelineBuildError>>>
  create_ctx_pipeline(
      entt::registry* world, const wgpu::Device& device,
      indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT vs_src_promise,
      indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT fs_src_promise,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list);
  static void update_pipeline(indigo::igecs::WorldView* wv,
                              const wgpu::Device& device);
  static const Pipeline& get_pipeline(indigo::igecs::WorldView* wv);

  // Geometry
  static ReadonlyResourceRegistry<Geo>::Key register_geo(
      indigo::igecs::WorldView* wv, const wgpu::Device& device,
      const indigo::core::PodVector<indigo::asset::PositionNormalVertexData>&
          vertices,
      const indigo::core::PodVector<uint32_t>& indices);
  static void unregister_geo(indigo::igecs::WorldView* wv,
                             ReadonlyResourceRegistry<Geo>::Key key);
  static void unregister_all_geo(indigo::igecs::WorldView* wv);

  // Rendering
  static void render_matching(indigo::igecs::WorldView* wv,
                              const wgpu::Device& device,
                              RenderUtil* render_util,
                              std::function<bool(entt::entity)> predicate);
};

}  // namespace sanctify::render::solid_static

#endif
