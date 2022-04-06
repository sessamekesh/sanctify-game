#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_COMPONENTS_TERRAIN_RENDER_COMPONENTS_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_COMPONENTS_TERRAIN_RENDER_COMPONENTS_H

#include <igasset/igpack_loader.h>
#include <igcore/vector.h>
#include <render/terrain/terrain_geo.h>
#include <render/terrain/terrain_instance_buffer_store.h>
#include <render/terrain/terrain_pipeline.h>
#include <util/resource_registry.h>

#include <entt/entt.hpp>
#include <unordered_map>

/**
 * Terrain renderable components go in here!
 */

namespace sanctify::ecs {

/** Information required to render an individual Terrain renderable component */
struct TerrainRenderableComponent {
  ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key geoKey;
  ReadonlyResourceRegistry<terrain_pipeline::MaterialPipelineInputs>::Key
      materialKey;
};

/**
 * Stores top-level context resources for terrain rendering - pipeline, resource
 * registries, index buffer registries.
 *
 * This context state is safe to create or emplace (default ctor okay)
 */
struct CtxTerrainRenderableResources {
  ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo> geoRegistry;
  ReadonlyResourceRegistry<terrain_pipeline::MaterialPipelineInputs>
      materialRegistry;
  terrain_pipeline::TerrainInstanceBufferStore instanceBufferStore;
};

/**
 * Stores top-level context resources for interacting with the terrain pipeline,
 * including the builder and current swap chain pipeline.
 *
 * Must be created during load time and MUST be present in order to use
 * get_pipeline (below)!
 */
struct CtxTerrainPipeline {
  terrain_pipeline::TerrainPipelineBuilder pipelineBuilder;
  indigo::core::Maybe<terrain_pipeline::TerrainPipeline> pipeline;
};

}  // namespace sanctify::ecs

#endif
