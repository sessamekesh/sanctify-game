#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_FACTORY_UTIL_BULID_TERRAIN_SHIT_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_FACTORY_UTIL_BULID_TERRAIN_SHIT_H

#include <game_scene/game_scene.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise.h>
#include <igcore/maybe.h>

namespace sanctify {

std::shared_ptr<
    indigo::core::Promise<indigo::core::Maybe<GameScene::TerrainShit>>>
load_terrain_shit(
    const wgpu::Device& device, const wgpu::TextureFormat& swap_chain_format,
    indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT vs_src_promise,
    indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT fs_src_promise,
    indigo::asset::IgpackLoader::ExtractDracoBufferPromiseT base_geo_promise,
    indigo::asset::IgpackLoader::ExtractDracoBufferPromiseT
        decoration_geo_promise,
    indigo::asset::IgpackLoader::ExtractDracoBufferPromiseT tower_geo_promise,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list);

}

#endif
