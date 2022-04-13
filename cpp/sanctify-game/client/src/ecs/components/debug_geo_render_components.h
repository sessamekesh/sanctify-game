#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_COMPONENTS_DEBUG_GEO_RENDER_COMPONENTS_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_COMPONENTS_DEBUG_GEO_RENDER_COMPONENTS_H

#include <render/debug_geo/debug_geo.h>
#include <render/debug_geo/debug_geo_pipeline.h>
#include <util/resource_registry.h>

/**
 * Engine-level context components for storing debug geo pipeline objects and
 *  resource registries for geometry+materials
 */

namespace sanctify::ecs {

/** Pipeline builder - asynchronously loaded, required to build pipeline */
struct CtxDebugGeoPipelineBuilder {
  debug_geo::DebugGeoPipelineBuilder pipelineBuilder;
};

/** Pipline object distinct from builder because swap chain format may change */
struct CtxDebugGeoPipeline {
  debug_geo::DebugGeoPipeline pipeline;
  wgpu::TextureFormat swapChainFormat;
};

/** Resource registries for managed debug geo resources */
struct CtxDebugGeoRenderableResources {
  ReadonlyResourceRegistry<debug_geo::DebugGeo> geoRegistry;
  debug_geo::DebugGeoInstanceBufferStore instanceBufferStore;
};

/**
 * Individual renderable - does not have MatWorld, that's contained in generic
 *  component
 */
struct DebugGeoRenderable {
  ReadonlyResourceRegistry<debug_geo::DebugGeo>::Key geoKey;
  glm::vec3 objectColor;
};

}  // namespace sanctify::ecs

#endif
