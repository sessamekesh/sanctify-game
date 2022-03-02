#ifndef SANCTIFY_GAME_CLIENT_SRC_UTIL_SCENE_SETUP_UTIL_H
#define SANCTIFY_GAME_CLIENT_SRC_UTIL_SCENE_SETUP_UTIL_H

#include <igasset/igpack_loader.h>
#include <igasync/promise.h>
#include <ozz/animation/runtime/animation.h>
#include <render/solid_animated/solid_animated_geo.h>
#include <render/solid_animated/solid_animated_pipeline.h>
#include <util/resource_registry.h>
#include <webgpu/webgpu_cpp.h>

/**
 * Utilities around setting up scene resource registries, etc
 */

namespace sanctify::util {

////////////////////////////////////////////////////////////////////////
// Registry Populating Utilities
////////////////////////////////////////////////////////////////////////

/**
 * Return type that contains both the key that will be set on the resource, as
 * well as a promise that will resolve once the resource has been successfully
 * loaded into the resource registry
 */
template <typename T>
struct LoadResourceToRegistryRsl {
  typename ReadonlyResourceRegistry<T>::Key key;
  std::shared_ptr<indigo::core::Promise<bool>> result_promise;
};

LoadResourceToRegistryRsl<ozz::animation::Animation> load_ozz_animation(
    const indigo::asset::IgpackLoader& loader, std::string igpack_name,
    const std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Animation>>&
        animation_registry,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list);

LoadResourceToRegistryRsl<ozz::animation::Skeleton> load_ozz_skeleton(
    const indigo::asset::IgpackLoader& loader, std::string igpack_name,
    const std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Skeleton>>&
        skeleton_registry,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list);

LoadResourceToRegistryRsl<solid_animated::SolidAnimatedGeo> load_geo(
    const wgpu::Device& device,
    const indigo::asset::IgpackLoader::ExtractDracoBufferPromiseT&
        geo_extract_promise,
    std::string debug_name,
    const std::shared_ptr<
        ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>>&
        geo_registry,
    const std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Skeleton>>&
        skeleton_registry,
    ReadonlyResourceRegistry<ozz::animation::Skeleton>::Key skeleton_key,
    const std::shared_ptr<indigo::core::Promise<bool>>
        skeleton_load_gate_promise,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list);

////////////////////////////////////////////////////////////////////////
// Shader Building Utilities
////////////////////////////////////////////////////////////////////////
std::shared_ptr<indigo::core::Promise<
    indigo::core::Maybe<solid_animated::SolidAnimatedPipelineBuilder>>>
load_solid_animated_pipeline(
    const wgpu::Device& device,
    const indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT&
        vs_src_promise,
    const indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT&
        fs_src_promise,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list);

}  // namespace sanctify::util

#endif
