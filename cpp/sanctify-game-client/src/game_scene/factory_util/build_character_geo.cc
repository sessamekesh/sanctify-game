#include <game_scene/factory_util/build_character_geo.h>
#include <igasync/promise_combiner.h>

using namespace indigo;
using namespace core;
using namespace asset;
using namespace sanctify;

namespace {
const char* kLogLabel = "GameSceneFactory::load_player_shit";
}

std::shared_ptr<indigo::core::Promise<Maybe<GameScene::PlayerShit>>>
sanctify::load_player_shit(
    const wgpu::Device& device, const wgpu::TextureFormat& swap_chain_format,
    const IgpackLoader::ExtractWgslShaderPromiseT& vs_src_promise,
    const IgpackLoader::ExtractWgslShaderPromiseT& fs_src_promise,
    const IgpackLoader::ExtractDracoBufferPromiseT& base_geo_promise,
    const IgpackLoader::ExtractDracoBufferPromiseT& joints_geo_promise,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list) {
  auto combiner = PromiseCombiner::Create();

  auto vs_src_key = combiner->add(vs_src_promise, async_task_list);
  auto fs_src_key = combiner->add(fs_src_promise, async_task_list);
  auto base_geo_key = combiner->add(base_geo_promise, async_task_list);
  auto joints_geo_key = combiner->add(joints_geo_promise, async_task_list);

  return combiner->combine()->then<Maybe<GameScene::PlayerShit>>(
      [device, main_thread_task_list, async_task_list, swap_chain_format,
       vs_src_key, fs_src_key, base_geo_key,
       joints_geo_key](const PromiseCombiner::PromiseCombinerResult& rsl)
          -> Maybe<GameScene::PlayerShit> {
        const IgpackLoader::ExtractWgslShaderT& vs_wgsl_src_rsl =
            rsl.get(vs_src_key);
        const IgpackLoader::ExtractWgslShaderT& fs_wgsl_src_rsl =
            rsl.get(fs_src_key);
        IgpackLoader::ExtractDracoBufferT base_geo_rsl = rsl.move(base_geo_key);
        IgpackLoader::ExtractDracoBufferT joint_geo_rsl =
            rsl.move(joints_geo_key);

        //
        // Process initial outputs for result errors - later code assumes all
        //  of these resources were successfully loaded
        //
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
        if (base_geo_rsl.is_right()) {
          Logger::err(kLogLabel) << "Base terrain geo load error - "
                                 << asset::to_string(base_geo_rsl.get_right());
          has_error = true;
        }
        if (joint_geo_rsl.is_right()) {
          Logger::err(kLogLabel) << "Terrain decorations geo load error - "
                                 << asset::to_string(joint_geo_rsl.get_right());
          has_error = true;
        }
        if (has_error) {
          return empty_maybe();
        }

        const asset::pb::WgslSource& vs_src = vs_wgsl_src_rsl.get_left();
        const asset::pb::WgslSource& fs_src = fs_wgsl_src_rsl.get_left();
        std::shared_ptr<asset::DracoDecoder> base_draco =
            base_geo_rsl.left_move();
        std::shared_ptr<asset::DracoDecoder> joint_draco =
            joint_geo_rsl.left_move();

        //
        // Extract geometry from Draco decoders - may also fail
        //
        auto base_verts_rsl = base_draco->get_pos_norm_data();
        auto base_indices_rsl = base_draco->get_index_data();
        auto joint_verts_rsl = joint_draco->get_pos_norm_data();
        auto joint_indices_rsl = joint_draco->get_index_data();

        has_error = false;
        if (base_verts_rsl.is_right()) {
          Logger::err(kLogLabel)
              << "Base geo vertices failed - "
              << asset::to_string(base_verts_rsl.get_right());
          has_error = true;
        }
        if (base_indices_rsl.is_right()) {
          Logger::err(kLogLabel)
              << "Base geo indices failed - "
              << asset::to_string(base_indices_rsl.get_right());
          has_error = true;
        }
        if (joint_verts_rsl.is_right()) {
          Logger::err(kLogLabel)
              << "Joints geo vertices failed - "
              << asset::to_string(joint_verts_rsl.get_right());
          has_error = true;
        }
        if (joint_indices_rsl.is_right()) {
          Logger::err(kLogLabel)
              << "Joints geo indices failed - "
              << asset::to_string(joint_indices_rsl.get_right());
          has_error = true;
        }
        if (has_error) {
          return empty_maybe();
        }

        PodVector<asset::PositionNormalVertexData> base_verts =
            base_verts_rsl.left_move();
        PodVector<uint32_t> base_indices = base_indices_rsl.left_move();
        PodVector<asset::PositionNormalVertexData> joint_verts =
            joint_verts_rsl.left_move();
        PodVector<uint32_t> joint_indices = joint_indices_rsl.left_move();

        //
        // Assemble all objects that go into the PlayerShit struct
        //
        auto maybe_pipeline_builder =
            solid_animated::SolidAnimatedPipelineBuilder::Create(device, vs_src,
                                                                 fs_src);

        if (maybe_pipeline_builder.is_empty()) {
          Logger::err(kLogLabel) << "Failed to create pipeline builder";
          return empty_maybe();
        }

        auto pipeline_builder = maybe_pipeline_builder.move();
        auto pipeline =
            pipeline_builder.create_pipeline(device, swap_chain_format);

        auto scene_inputs = pipeline.create_scene_inputs(
            device, glm::normalize(glm::vec3(1.f, -3.f, 1.f)),
            glm::vec3(1.f, 1.f, 1.f), 0.3f, 50.f);
        auto surface_material = pipeline.create_material_inputs(
            device, glm::vec3(0.628f, 0.628f, 1.f));
        auto joints_material = pipeline.create_material_inputs(
            device, glm::vec3(0.1f, 0.1f, 0.2f));
        auto frame_inputs = pipeline.create_frame_inputs(device);

        solid_animated::SolidAnimatedGeo base_geo(device, base_verts,
                                                  base_indices);
        solid_animated::SolidAnimatedGeo joints_geo(device, joint_verts,
                                                    joint_indices);

        core::PodVector<solid_animated::MatWorldInstanceData> identity_instance(
            1);
        // TODO (sessamekesh): this rotation will be a common case for Assimp
        // files, you should handle this in pre-processing (igpack-gen)
        glm::mat4 make_tiny =
            glm::scale(glm::mat4(1.f), glm::vec3(0.06f, 0.06f, 0.06f));
        identity_instance.push_back({make_tiny});

        solid_animated::MatWorldInstanceBuffer identity_buffer(
            device, identity_instance);

        return GameScene::PlayerShit{
            std::move(pipeline_builder), std::move(pipeline),
            std::move(frame_inputs),     std::move(scene_inputs),
            std::move(surface_material), std::move(joints_material),
            std::move(base_geo),         std::move(joints_geo),
            std::move(identity_buffer)};
      },
      main_thread_task_list);
}