#include <game_scene/factory_util/build_terrain_shit.h>
#include <igasync/promise_combiner.h>

using namespace indigo;
using namespace core;
using namespace asset;
using namespace sanctify;

std::shared_ptr<Promise<Maybe<GameScene::TerrainShit>>>
sanctify::load_terrain_shit(
    const wgpu::Device& device, const wgpu::TextureFormat& swap_chain_format,
    IgpackLoader::ExtractWgslShaderPromiseT vs_src_promise,
    IgpackLoader::ExtractWgslShaderPromiseT fs_src_promise,
    IgpackLoader::ExtractDracoBufferPromiseT base_geo_promise,
    IgpackLoader::ExtractDracoBufferPromiseT decoration_geo_promise,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list) {
  auto combiner = PromiseCombiner::Create();

  auto vs_src_key = combiner->add(vs_src_promise, async_task_list);
  auto fs_src_key = combiner->add(fs_src_promise, async_task_list);
  auto base_geo_key = combiner->add(base_geo_promise, async_task_list);
  auto decoration_geo_key =
      combiner->add(decoration_geo_promise, async_task_list);

  return combiner->combine()->then<Maybe<GameScene::TerrainShit>>(
      [device, main_thread_task_list, swap_chain_format, vs_src_key, fs_src_key,
       base_geo_key,
       decoration_geo_key](const PromiseCombiner::PromiseCombinerResult& rsl)
          -> Maybe<GameScene::TerrainShit> {
        const char* kTerrainShitLabel = "GameSceneFactory::load_terrain_shit";

        IgpackLoader::ExtractWgslShaderT vs_wgsl_src_rsl = rsl.move(vs_src_key);
        IgpackLoader::ExtractWgslShaderT fs_wgsl_src_rsl = rsl.move(fs_src_key);
        IgpackLoader::ExtractDracoBufferT base_geo_rsl = rsl.move(base_geo_key);
        IgpackLoader::ExtractDracoBufferT decoration_geo_rsl =
            rsl.move(decoration_geo_key);

        //
        // Process the initial outputs for result errors - later code assumes
        // all igpack resource extraction succeeded
        //
        bool has_error = false;
        if (vs_wgsl_src_rsl.is_right()) {
          Logger::err(kTerrainShitLabel)
              << "VS WGSL source load error - "
              << asset::to_string(vs_wgsl_src_rsl.get_right());
          has_error = true;
        }
        if (fs_wgsl_src_rsl.is_right()) {
          Logger::err(kTerrainShitLabel)
              << "FS WGSL source load error - "
              << asset::to_string(fs_wgsl_src_rsl.get_right());
          has_error = true;
        }
        if (base_geo_rsl.is_right()) {
          Logger::err(kTerrainShitLabel)
              << "Base terrain geo load error - "
              << asset::to_string(base_geo_rsl.get_right());
          has_error = true;
        }
        if (decoration_geo_rsl.is_right()) {
          Logger::err(kTerrainShitLabel)
              << "Terrain decorations geo load error - "
              << asset::to_string(decoration_geo_rsl.get_right());
          has_error = true;
        }
        if (has_error) {
          return empty_maybe();
        }
        asset::pb::WgslSource vs_src = vs_wgsl_src_rsl.left_move();
        asset::pb::WgslSource fs_src = fs_wgsl_src_rsl.left_move();
        std::shared_ptr<asset::DracoDecoder> base_draco =
            base_geo_rsl.left_move();
        std::shared_ptr<asset::DracoDecoder> decoration_draco =
            decoration_geo_rsl.left_move();

        //
        // Pull geometry data out of Draco decoders - may also fail.
        //
        auto base_geo_vertices = base_draco->get_pos_norm_data();
        auto base_geo_indices = base_draco->get_index_data();
        auto decoration_geo_vertices = decoration_draco->get_pos_norm_data();
        auto decoration_geo_indices = decoration_draco->get_index_data();

        has_error = false;
        if (base_geo_vertices.is_right()) {
          Logger::err(kTerrainShitLabel)
              << "Base geo vertices failed - "
              << asset::to_string(base_geo_vertices.get_right());
          has_error = true;
        }
        if (base_geo_indices.is_right()) {
          Logger::err(kTerrainShitLabel)
              << "Base geo indices failed - "
              << asset::to_string(base_geo_indices.get_right());
          has_error = true;
        }
        if (decoration_geo_vertices.is_right()) {
          Logger::err(kTerrainShitLabel)
              << "Decoration geo vertices failed - "
              << asset::to_string(decoration_geo_vertices.get_right());
          has_error = true;
        }
        if (decoration_geo_indices.is_right()) {
          Logger::err(kTerrainShitLabel)
              << "Decoration geo indices failed - "
              << asset::to_string(decoration_geo_indices.get_right());
          has_error = true;
        }
        if (has_error) {
          return empty_maybe();
        }
        core::PodVector<asset::PositionNormalVertexData> base_verts =
            base_geo_vertices.left_move();
        core::PodVector<uint32_t> base_indices = base_geo_indices.left_move();
        core::PodVector<asset::PositionNormalVertexData> decoration_verts =
            decoration_geo_vertices.left_move();
        core::PodVector<uint32_t> decoration_indices =
            decoration_geo_indices.left_move();

        //
        // Assemble all the objects that go in the TerrainShit struct
        //
        Maybe<terrain_pipeline::TerrainPipelineBuilder> maybe_pipeline_builder =
            terrain_pipeline::TerrainPipelineBuilder::Create(device, vs_src,
                                                             fs_src);

        if (maybe_pipeline_builder.is_empty()) {
          Logger::err(kTerrainShitLabel) << "Failed to create pipeline builder";
          return empty_maybe();
        }

        terrain_pipeline::TerrainPipelineBuilder pipeline_builder =
            maybe_pipeline_builder.move();

        terrain_pipeline::TerrainPipeline pipeline =
            pipeline_builder.create_pipeline(device, swap_chain_format);

        terrain_pipeline::ScenePipelineInputs scene_inputs =
            pipeline.create_scene_inputs(
                device, glm::normalize(glm::vec3(1.f, -3.f, 1.f)),
                glm::vec3(1.f, 1.f, 1.f), 0.3f, 50.f);

        terrain_pipeline::MaterialPipelineInputs material_inputs =
            pipeline.create_material_inputs(device,
                                            glm::vec3(0.828f, 0.828f, 0.85f));

        terrain_pipeline::FramePipelineInputs frame_inputs =
            pipeline.create_frame_inputs(device);

        terrain_pipeline::TerrainGeo base_geo(device, base_verts, base_indices);
        terrain_pipeline::TerrainGeo decoration_geo(device, decoration_verts,
                                                    decoration_indices);
        core::PodVector<terrain_pipeline::TerrainMatWorldInstanceData>
            identity_instance(0);
        // TODO (sessamekesh): this rotation will be a common case for Assimp
        // files, you should handle this in pre-processing (igpack-gen)
        glm::mat4 rotate_i_guess = glm::rotate(
            glm::mat4(1.f), glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
        identity_instance.push_back({rotate_i_guess});
        terrain_pipeline::TerrainMatWorldInstanceBuffer identity_buffer(
            device, identity_instance);

        return GameScene::TerrainShit{
            std::move(pipeline_builder), std::move(pipeline),
            std::move(frame_inputs),     std::move(scene_inputs),
            std::move(material_inputs),  std::move(base_geo),
            std::move(decoration_geo),   std::move(identity_buffer)};
      },
      main_thread_task_list);
}