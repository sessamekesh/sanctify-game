#ifndef SANCTIFY_GAME_COMMON_SRC_UTIL_REGISTRY_TYPES_H
#define SANCTIFY_GAME_COMMON_SRC_UTIL_REGISTRY_TYPES_H

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <render/solid_animated/solid_animated_geo.h>
#include <render/solid_animated/solid_animated_pipeline.h>
#include <util/resource_registry.h>

namespace sanctify {

using OzzSkeletonRegistry =
    std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Skeleton>>;
using OzzAnimationRegistry =
    std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Animation>>;
using SolidAnimatedGeoRegistry =
    std::shared_ptr<ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>>;
using SolidAnimatedMaterialRegistry = std::shared_ptr<
    ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>>;

}  // namespace sanctify

#endif
