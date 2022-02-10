#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_FACTORY_UTIL_BUILD_DEBUG_GEO_SHIT_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_FACTORY_UTIL_BUILD_DEBUG_GEO_SHIT_H

#include <game_scene/game_scene.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise.h>

namespace sanctify {

std::shared_ptr<
    indigo::core::Promise<indigo::core::Maybe<GameScene::DebugGeoShit>>>
load_debug_geo_shit(
    const wgpu::Device& device, const wgpu::TextureFormat& swap_chain_format,
    const indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT&
        vs_src_promise,
    const indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT&
        fs_src_promise,
    std::shared_ptr<indigo::core::TaskList> main_thread_taks_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list);

}

#endif
