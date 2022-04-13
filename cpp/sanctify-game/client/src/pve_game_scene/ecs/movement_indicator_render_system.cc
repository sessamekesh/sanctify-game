#include "movement_indicator_render_system.h"

#include <ecs/components/common_render_components.h>
#include <ecs/components/debug_geo_render_components.h>
#include <pve_game_scene/ecs/client_config.h>
#include <pve_game_scene/ecs/utils.h>
#include <pve_game_scene/render/debug_geo_resources.h>
#include <render/debug_geo/debug_geo.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace sanctify;
using namespace pve;

namespace {
struct MovementIndicator {
  float t;
  glm::vec3 location;
};

ecs::DebugGeoRenderable& get_renderable(entt::registry& world, entt::entity e) {
  ecs::DebugGeoRenderable* renderable =
      world.try_get<ecs::DebugGeoRenderable>(e);
  if (renderable != nullptr) {
    return *renderable;
  }

  auto& resources = world.ctx<pve::CtxDebugGeoGeoResources>();
  return world.emplace<ecs::DebugGeoRenderable>(e, resources.unitCubeKey);
}

ecs::MatWorldComponent& get_mat_world(entt::registry& world, entt::entity e) {
  ecs::MatWorldComponent* c = world.try_get<ecs::MatWorldComponent>(e);

  if (c != nullptr) {
    return *c;
  }

  return world.emplace<ecs::MatWorldComponent>(e, glm::mat4(1.f));
}

}  // namespace

bool MovementIndicatorRenderSystem::is_ready(entt::registry& world) {
  bool has_debug_geo_geo_resources =
      world.try_ctx<pve::CtxDebugGeoGeoResources>();

  return has_debug_geo_geo_resources;
}

void MovementIndicatorRenderSystem::add_indicator(entt::registry& world,
                                                  glm::vec2 position) {
  auto& config = world.ctx_or_set<ClientConfigComponent>();

  auto e = world.create();
  world.emplace<::MovementIndicator>(
      e, config.moveIndicatorRenderParams.lifetimeSeconds,
      glm::vec3(position.x, 0.5f, position.y));
}

void MovementIndicatorRenderSystem::update(entt::registry& world, float dt) {
  auto& config = world.ctx_or_set<ClientConfigComponent>();

  auto view = world.view<::MovementIndicator>();

  for (auto [e, movement_indicator] : view.each()) {
    movement_indicator.t -= dt;
    if (movement_indicator.t < 0.f) {
      world.destroy(e);
      continue;
    }

    auto& renderable = ::get_renderable(world, e);
    auto& mat_world_component = ::get_mat_world(world, e);

    float t = 1.f - movement_indicator.t /
                        config.moveIndicatorRenderParams.lifetimeSeconds;
    float scale = t * config.moveIndicatorRenderParams.endScale +
                  (1.f - t) * config.moveIndicatorRenderParams.startScale;
    glm::vec3 color = t * config.moveIndicatorRenderParams.endColor +
                      (1.f - t) * config.moveIndicatorRenderParams.startColor;

    mat_world_component.matWorld =
        glm::translate(glm::mat4(1.f), movement_indicator.location);
    mat_world_component.matWorld =
        glm::scale(mat_world_component.matWorld, glm::vec3(scale));
    renderable.objectColor = color;
  }
}
