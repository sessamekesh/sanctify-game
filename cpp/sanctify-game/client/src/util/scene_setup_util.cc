#include <igasync/promise_combiner.h>
#include <igcore/either.h>
#include <igcore/log.h>
#include <util/scene_setup_util.h>

using namespace sanctify;
using namespace util;
using namespace indigo;
using namespace core;
using namespace asset;

namespace {
const char* kLogLabel = "SceneSetupUtil";

//
// Errors for extracting draco geo
//
const uint8_t kNoData = 0x01;
const uint8_t kNoPos = 0x02;
const uint8_t kNoAnim = 0x04;
const uint8_t kNoIndices = 0x08;
const uint8_t kNoSkeleton = 0x10;
const uint8_t kNoInvBindPoses = 0x20;

void log_animated_draco_extract_errors(uint8_t errs) {
  auto log = Logger::err(kLogLabel);
  log << "Error extracting animated Draco errors:";
  if (errs & kNoData) {
    log << "\n-- Draco object was not created";
  }
  if (errs & kNoPos) {
    log << "\n-- No position data available";
  }
  if (errs & kNoAnim) {
    log << "\n-- Animation data not available";
  }
  if (errs & kNoIndices) {
    log << "\n-- No geometry indices available";
  }
  if (errs & kNoSkeleton) {
    log << "\n-- No skeleton data found in registry";
  }
  if (errs & kNoInvBindPoses) {
    log << "\n-- No inverse bind poses found in registry";
  }
}

//
// Solid animated pipeline extract errors
//
const uint8_t kNoVsData = 0x01;
const uint8_t kNoFsData = 0x02;
void log_build_pipeline_errors(uint8_t errs) {
  auto log = Logger::err(kLogLabel);
  log << "Error extracting solid animated pipeline:";
  if (errs & kNoVsData) {
    log << "\n-- No vertex shader source available";
  }
  if (errs & kNoFsData) {
    log << "\n-- No fragment shader source available";
  }
}
}  // namespace

LoadResourceToRegistryRsl<ozz::animation::Animation> util::load_ozz_animation(
    const IgpackLoader& loader, std::string igpack_name,
    const std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Animation>>&
        animation_registry,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list) {
  using KeyType = ReadonlyResourceRegistry<ozz::animation::Animation>::Key;

  KeyType key = animation_registry->reserve_resource();
  auto rsl_promise =
      loader.extract_ozz_animation(igpack_name, async_task_list)
          ->then_consuming<bool>(
              [igpack_name, key, animation_registry](
                  IgpackLoader::ExtractOzzAnimationT rsl) -> bool {
                if (rsl.is_right()) {
                  Logger::err(kLogLabel)
                      << "Failed to extract animation " << igpack_name << " - "
                      << ::to_string(rsl.get_right());
                  return false;
                }

                return animation_registry->set_reserved_resource(
                    key, rsl.left_move());
              },
              main_thread_task_list);

  return {key, rsl_promise};
}

LoadResourceToRegistryRsl<ozz::animation::Skeleton> util::load_ozz_skeleton(
    const IgpackLoader& loader, std::string igpack_name,
    const std::shared_ptr<ReadonlyResourceRegistry<ozz::animation::Skeleton>>&
        skeleton_registry,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list) {
  using KeyType = ReadonlyResourceRegistry<ozz::animation::Skeleton>::Key;

  KeyType key = skeleton_registry->reserve_resource();
  auto rsl_promise =
      loader.extract_ozz_skeleton(igpack_name, async_task_list)
          ->then_consuming<bool>(
              [igpack_name, key, skeleton_registry](
                  IgpackLoader::ExtractOzzSkeletonT rsl) -> bool {
                if (rsl.is_right()) {
                  Logger::err(kLogLabel)
                      << "Failed to extract skeleton " << igpack_name << " - "
                      << ::to_string(rsl.get_right());
                  return false;
                }

                return skeleton_registry->set_reserved_resource(
                    key, rsl.left_move());
              },
              main_thread_task_list);

  return {key, rsl_promise};
}

LoadResourceToRegistryRsl<solid_animated::SolidAnimatedGeo> util::load_geo(
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
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list) {
  using KeyType =
      ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key;

  struct FullAnimatedVertexData {
    PodVector<asset::PositionNormalVertexData> posNormData;
    PodVector<asset::SkeletalAnimationVertexData> animationData;
    PodVector<glm::mat4> invBindPoseData;
    PodVector<uint32_t> indices;
  };

  KeyType key = geo_registry->reserve_resource();
  auto rsl_promise = skeleton_load_gate_promise->then_chain<bool>(
      [debug_name, skeleton_registry, skeleton_key, async_task_list,
       main_thread_task_list, key, geo_extract_promise, geo_registry,
       device](const auto& rsl) -> std::shared_ptr<Promise<bool>> {
        return geo_extract_promise
            ->then<Either<FullAnimatedVertexData, uint8_t>>(
                [debug_name, skeleton_registry,
                 skeleton_key](const IgpackLoader::ExtractDracoBufferT& rsl)
                    -> Either<FullAnimatedVertexData, uint8_t> {
                  uint8_t errs = 0x00;
                  if (rsl.is_right()) {
                    Logger::err(kLogLabel)
                        << "Failed to extract Draco geo from " << debug_name
                        << ": " << ::to_string(rsl.get_right());
                    errs = errs | kNoData;
                  }

                  const ozz::animation::Skeleton* skeleton =
                      skeleton_registry->get(skeleton_key);
                  if (skeleton == nullptr) {
                    errs = errs | kNoSkeleton;
                  }

                  if (errs > 0) {
                    log_animated_draco_extract_errors(errs);
                    return right<uint8_t>(errs);
                  }

                  std::shared_ptr<asset::DracoDecoder> draco_decoder =
                      rsl.get_left();

                  auto pos_norm_verts_rsl = draco_decoder->get_pos_norm_data();
                  auto anim_verts_rsl =
                      draco_decoder->get_skeletal_animation_vertices(*skeleton);
                  auto indices_rsl = draco_decoder->get_index_data();
                  auto inv_bind_poses_rsl =
                      draco_decoder->get_inv_bind_poses(*skeleton);

                  if (pos_norm_verts_rsl.is_right()) {
                    Logger::err(kLogLabel)
                        << "Failed to extract pos-norm data from " << debug_name
                        << ": " << ::to_string(pos_norm_verts_rsl.get_right());
                    errs |= kNoPos;
                  }

                  if (anim_verts_rsl.is_right()) {
                    Logger::err(kLogLabel)
                        << "Failed to extract anim-vert data from "
                        << debug_name << ": "
                        << ::to_string(pos_norm_verts_rsl.get_right());
                    errs |= kNoAnim;
                  }

                  if (indices_rsl.is_right()) {
                    Logger::err(kLogLabel)
                        << "Failed to extract index data from " << debug_name
                        << ": " << ::to_string(pos_norm_verts_rsl.get_right());
                    errs |= kNoIndices;
                  }

                  if (inv_bind_poses_rsl.is_right()) {
                    Logger::err(kLogLabel)
                        << "Failed to extract inverse bind poses from "
                        << debug_name << ": "
                        << ::to_string(inv_bind_poses_rsl.get_right());
                    errs |= kNoInvBindPoses;
                  }

                  if (errs > 0) {
                    log_animated_draco_extract_errors(errs);
                    return right<uint8_t>(errs);
                  }

                  return left(FullAnimatedVertexData{
                      pos_norm_verts_rsl.left_move(),
                      anim_verts_rsl.left_move(),
                      inv_bind_poses_rsl.left_move(), indices_rsl.left_move()});
                },
                async_task_list)
            ->then_consuming<bool>(
                [debug_name, key, geo_registry,
                 device](Either<FullAnimatedVertexData, uint8_t> rsl) -> bool {
                  if (rsl.is_right()) {
                    return false;
                  }

                  FullAnimatedVertexData geo_data = rsl.left_move();

                  return geo_registry->set_reserved_resource(
                      key, solid_animated::SolidAnimatedGeo(
                               device, geo_data.posNormData,
                               geo_data.animationData, geo_data.indices,
                               std::move(geo_data.invBindPoseData)));
                },
                main_thread_task_list);
      },
      main_thread_task_list);
  return {key, rsl_promise};
}

std::shared_ptr<Promise<Maybe<solid_animated::SolidAnimatedPipelineBuilder>>>
util::load_solid_animated_pipeline(
    const wgpu::Device& device,
    const IgpackLoader::ExtractWgslShaderPromiseT& vs_src_promise,
    const IgpackLoader::ExtractWgslShaderPromiseT& fs_src_promise,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list) {
  auto combiner = PromiseCombiner::Create();

  auto vs_src_key = combiner->add(vs_src_promise, async_task_list);
  auto fs_src_key = combiner->add(fs_src_promise, async_task_list);

  return combiner->combine()
      ->then<Maybe<solid_animated::SolidAnimatedPipelineBuilder>>(
          [device, async_task_list, vs_src_key,
           fs_src_key](const PromiseCombiner::PromiseCombinerResult& rsl)
              -> Maybe<solid_animated::SolidAnimatedPipelineBuilder> {
            const auto& vs_wgsl_src_rsl = rsl.get(vs_src_key);
            const auto& fs_wgsl_src_rsl = rsl.get(fs_src_key);

            uint8_t errs = 0x00;
            if (vs_wgsl_src_rsl.is_right()) {
              Logger::err(kLogLabel)
                  << "Failed to extract solid animated vertex shader source: "
                  << ::to_string(vs_wgsl_src_rsl.get_right());
              errs |= kNoVsData;
            }
            if (fs_wgsl_src_rsl.is_right()) {
              Logger::err(kLogLabel)
                  << "Failed to extract solid animated fragment shader source: "
                  << ::to_string(fs_wgsl_src_rsl.get_right());
              errs |= kNoFsData;
            }

            if (errs > 0x00u) {
              ::log_build_pipeline_errors(errs);
              return empty_maybe{};
            }

            const asset::pb::WgslSource& vs_src = vs_wgsl_src_rsl.get_left();
            const asset::pb::WgslSource& fs_src = fs_wgsl_src_rsl.get_left();

            return solid_animated::SolidAnimatedPipelineBuilder::Create(
                device, vs_src, fs_src);
          },
          main_thread_task_list);
}
