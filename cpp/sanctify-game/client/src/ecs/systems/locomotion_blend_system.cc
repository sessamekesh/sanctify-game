#include <ecs/systems/locomotion_blend_system.h>

using namespace sanctify;
using namespace ecs;

PositionSmearNetStateComponent::PositionSmearNetStateComponent(
    glm::vec2 smear_diff, float min_speed, float min_close_rate)
    : mapPosDiffDirection(glm::normalize(smear_diff)),
      diffMagnitude(glm::length(smear_diff)),
      minSmearSpeed(min_speed),
      minCloseRate(min_close_rate) {
  assert(min_speed > 0.f);
  assert(min_close_rate > 0.f);
}

void PositionSmearNetUpdateSystem::update(entt::registry& world, float dt) {
  auto view = world.view<PositionSmearNetStateComponent>();

  for (auto [e, smear] : view.each()) {
    float linear_smear = smear.minSmearSpeed * dt;
    // This could be a power for more realistic results, but we don't really
    //  need realism here so this will do just fine with the cheaper multiply
    float pct_smear = smear.minCloseRate * smear.diffMagnitude * dt;

    smear.diffMagnitude -= glm::max(linear_smear, pct_smear);
    if (smear.diffMagnitude <= 0.f) {
      world.remove<PositionSmearNetStateComponent>(e);
    }
  }
}
