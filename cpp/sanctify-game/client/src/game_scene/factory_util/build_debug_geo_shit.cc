#include <game_scene/factory_util/build_debug_geo_shit.h>
#include <igasync/promise_combiner.h>

using namespace indigo;
using namespace core;
using namespace asset;
using namespace sanctify;

namespace {
const char* kLogLabel = "GameSceneFactory::load_debug_geo_shit";
}

std::shared_ptr<
    indigo::core::Promise<indigo::core::Maybe<GameScene::DebugGeoShit>>>
sanctify::load_debug_geo_shit(
    const wgpu::Device& device, const wgpu::TextureFormat& swap_chain_format,
    const indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT&
        vs_src_promise,
    const indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT&
        fs_src_promise,
    std::shared_ptr<indigo::core::TaskList> main_thread_taks_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list) {
  auto combiner = PromiseCombiner::Create();

  auto vs_src_key = combiner->add(vs_src_promise, async_task_list);
  auto fs_src_key = combiner->add(fs_src_promise, async_task_list);

  return combiner->combine()->then<Maybe<GameScene::DebugGeoShit>>(
      [device, main_thread_taks_list, async_task_list, swap_chain_format,
       vs_src_key,
       fs_src_key](const PromiseCombiner::PromiseCombinerResult& rsl)
          -> Maybe<GameScene::DebugGeoShit> {
        const IgpackLoader::ExtractWgslShaderT& vs_wgsl_src_rsl =
            rsl.get(vs_src_key);
        const IgpackLoader::ExtractWgslShaderT& fs_wgsl_src_rsl =
            rsl.get(fs_src_key);

        bool has_error = false;
        if (vs_wgsl_src_rsl.is_right()) {
          Logger::err(kLogLabel)
              << "VS WGSL load error - "
              << asset::to_string(vs_wgsl_src_rsl.get_right());
          has_error = true;
        }
        if (fs_wgsl_src_rsl.is_right()) {
          Logger::err(kLogLabel)
              << "FS WGSL load error - "
              << asset::to_string(fs_wgsl_src_rsl.get_right());
          has_error = true;
        }
        if (has_error) {
          return empty_maybe();
        }

        const asset::pb::WgslSource& vs_src = vs_wgsl_src_rsl.get_left();
        const asset::pb::WgslSource& fs_src = fs_wgsl_src_rsl.get_left();

        auto cube_geo = debug_geo::DebugGeo::CreateDebugUnitCube(device);
        debug_geo::InstanceBuffer cube_instances(device);

        auto maybe_pipeline_builder =
            debug_geo::DebugGeoPipelineBuilder::Create(device, vs_src, fs_src);
        if (maybe_pipeline_builder.is_empty()) {
          Logger::err(kLogLabel) << "Failed to create pipeline builder";
          return empty_maybe{};
        }

        auto pipeline_builder = maybe_pipeline_builder.move();

        auto pipeline =
            pipeline_builder.create_pipeline(device, swap_chain_format);

        // TODO (sessamekesh): Inject this as some sort of app configuration
        auto scene_inputs = pipeline.create_scene_inputs(
            device, glm::normalize(glm::vec3(1.f, -3.f, 1.f)),
            glm::vec3(1.f, 1.f, 1.f), 0.5f, 50.f);
        auto frame_inputs = pipeline.create_frame_inputs(device);

        return GameScene::DebugGeoShit{
            std::move(pipeline_builder), std::move(pipeline),
            std::move(frame_inputs),     std::move(scene_inputs),
            std::move(cube_geo),         std::move(cube_instances)};
      },
      main_thread_taks_list);
}
