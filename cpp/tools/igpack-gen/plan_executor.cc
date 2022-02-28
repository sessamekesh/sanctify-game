#include <igcore/log.h>
#include <plan_executor.h>

#include <fstream>

namespace {
const char* kLogLabel = "PlanExecutor";
}

using namespace indigo;
using namespace igpackgen;

PlanExecutor::PlanExecutor(uint32_t max_file_memory_cache)
    : max_file_memory_cache_(max_file_memory_cache),
      assimp_geo_processor_(),
      recast_navmesh_processor_() {}

bool PlanExecutor::execute_plan(const PlanInvocationDesc& desc) {
  // Validate all inputs are present
  for (int i = 0; i < desc.Plan.plan_size(); i++) {
    if (!validate_inputs_exist(desc.InputAssetPathRoot, desc.Plan.plan(i))) {
      core::Logger::err(kLogLabel) << "Error - input plan " << i
                                   << " contains inputs that cannot be found";
      return false;
    }
  }

  FileCache file_cache(desc.InputAssetPathRoot, max_file_memory_cache_);
  AssimpSceneCache assimp_scene_cache(max_file_memory_cache_);

  // Pre-register which skeleton bones will be used from each animation in the
  //  file, and make sure that all skeletons loaded are (1) used by animations,
  //  and (2) well-formed
  for (int plan_idx = 0; plan_idx < desc.Plan.plan_size(); plan_idx++) {
    for (int action_idx = 0;
         action_idx < desc.Plan.plan(plan_idx).actions_size(); action_idx++) {
      const pb::SingleAction& action =
          desc.Plan.plan(plan_idx).actions(action_idx);
      if (action.has_extract_ozz_animation()) {
        if (!assimp_animation_processor_.preload_animation_bones(
                action.extract_ozz_animation(), file_cache,
                assimp_scene_cache)) {
          core::Logger::err(kLogLabel)
              << "Failed to pre-validate animations for plan "
              << desc.Plan.plan(plan_idx).asset_pack_file_path();
          return false;
        }
      }
    }
  }

  // Once all animations have had their say on which bones are used, make sure
  //  all the loaded skeletons also comply.
  for (int plan_idx = 0; plan_idx < desc.Plan.plan_size(); plan_idx++) {
    for (int action_idx = 0;
         action_idx < desc.Plan.plan(plan_idx).actions_size(); action_idx++) {
      const pb::SingleAction& action =
          desc.Plan.plan(plan_idx).actions(action_idx);
      if (action.has_extract_ozz_skeleton()) {
        if (!assimp_animation_processor_.validate_bones_exist(
                action.extract_ozz_skeleton(), file_cache,
                assimp_scene_cache)) {
          core::Logger::err(kLogLabel)
              << "Failed to pre-validate animations for plan "
              << desc.Plan.plan(plan_idx).asset_pack_file_path();
          return false;
        }
      }
    }
  }

  for (int plan_idx = 0; plan_idx < desc.Plan.plan_size(); plan_idx++) {
    const pb::SingleIgpackPlan& plan = desc.Plan.plan(plan_idx);

    asset::pb::AssetPack out_asset_pack;

    for (int action_idx = 0; action_idx < plan.actions_size(); action_idx++) {
      const pb::SingleAction& action = plan.actions(action_idx);

      switch (action.request_case()) {
        case pb::SingleAction::kCopyWgslSource:
          if (!copy_wgsl_source(out_asset_pack, action.copy_wgsl_source(),
                                desc.InputAssetPathRoot, file_cache)) {
            core::Logger::err(kLogLabel)
                << "Failed to convert WGSL from source "
                << action.copy_wgsl_source().input_file_path();
            return false;
          }
          break;
        case pb::SingleAction::kAssimpToStaticDracoGeo:
          if (!convert_assimp_file(out_asset_pack,
                                   action.assimp_to_static_draco_geo(),
                                   file_cache, assimp_scene_cache)) {
            core::Logger::err(kLogLabel)
                << "Failed to convert Assimp from source "
                << action.assimp_to_static_draco_geo().input_file_path();
            return false;
          }
          break;
        case pb::SingleAction::kAssembleNavmesh:
          if (!assemble_navmesh(out_asset_pack, action.assemble_navmesh(),
                                file_cache, assimp_scene_cache)) {
            core::Logger::err(kLogLabel) << "Failed to assemble navmesh";
            return false;
          }
          break;
        case pb::SingleAction::kExtractSkinnedDracoGeo:
          if (!convert_skinned_assimp_file(out_asset_pack,
                                           action.extract_skinned_draco_geo(),
                                           file_cache, assimp_scene_cache)) {
            core::Logger::err(kLogLabel)
                << "Failed to extract skinned mesh from Assimp source "
                << action.assimp_to_static_draco_geo().input_file_path();
            return false;
          }
          break;
        case pb::SingleAction::kExtractOzzAnimation:
          if (!create_and_export_animation(out_asset_pack,
                                           action.extract_ozz_animation(),
                                           file_cache, assimp_scene_cache)) {
            core::Logger::err(kLogLabel)
                << "Failed to extract Ozz animation "
                << action.extract_ozz_animation().animation_igasset_name()
                << " from Assimp source";
            return false;
          }
          break;
        case pb::SingleAction::kExtractOzzSkeleton:
          if (!create_and_export_skeleton(out_asset_pack,
                                          action.extract_ozz_skeleton(),
                                          file_cache, assimp_scene_cache)) {
            core::Logger::err(kLogLabel)
                << "Failed to extract Ozz skeleton "
                << action.extract_ozz_skeleton().skeleton_igasset_name()
                << " from Assimp source";
            return false;
          }
          break;
        default:
          core::Logger::err(kLogLabel)
              << "Warning - plan " << plan_idx << ", action " << action_idx
              << " is an unrecognized type";
          return false;
      }
    }

    std::string pack_bin = out_asset_pack.SerializeAsString();
    if (pack_bin.size() == 0) {
      core::Logger::err(kLogLabel)
          << "Failed to serialize asset pack " << plan.asset_pack_file_path();
      return false;
    }

    std::filesystem::path out_path =
        desc.OutputAssetPathRoot / plan.asset_pack_file_path();
    std::filesystem::path out_dir = out_path;
    out_dir.remove_filename();
    if (!std::filesystem::exists(out_dir)) {
      std::filesystem::create_directories(out_dir);
    }

    std::ofstream fout(desc.OutputAssetPathRoot / plan.asset_pack_file_path(),
                       std::fstream::binary);
    if (!fout.write(&pack_bin[0], pack_bin.size())) {
      core::Logger::err(kLogLabel)
          << "Failed to write plan " << plan.asset_pack_file_path()
          << " to file path "
          << desc.OutputAssetPathRoot / plan.asset_pack_file_path();
      return false;
    }
  }

  return true;
}

bool PlanExecutor::validate_inputs_exist(
    std::filesystem::path input_root, const pb::SingleIgpackPlan& plan) const {
  for (int i = 0; i < plan.actions_size(); i++) {
    const pb::SingleAction& action = plan.actions(i);

    std::set<std::string> skeleton_names;

    switch (action.request_case()) {
      case pb::SingleAction::kCopyWgslSource:
        if (!peek_file(input_root,
                       action.copy_wgsl_source().input_file_path())) {
          core::Logger::err(kLogLabel)
              << "WGSL file not found: "
              << input_root / action.copy_wgsl_source().input_file_path();
          return false;
        }
        break;
      case pb::SingleAction::kAssimpToStaticDracoGeo:
        if (!peek_file(input_root,
                       action.assimp_to_static_draco_geo().input_file_path())) {
          core::Logger::err(kLogLabel)
              << "Assimp file not found: "
              << input_root /
                     action.assimp_to_static_draco_geo().input_file_path();
          return false;
        }
        break;
      case pb::SingleAction::kAssembleNavmesh:
        for (const auto& op : action.assemble_navmesh().recast_build_ops()) {
          if (op.has_include_assimp_geo() &&
              !peek_file(input_root,
                         op.include_assimp_geo().assimp_file_name())) {
            core::Logger::err(kLogLabel)
                << "Assimp file not found: "
                << input_root / op.include_assimp_geo().assimp_file_name();
            return false;
          }
        }
        break;
      case pb::SingleAction::kExtractOzzAnimation:
        if (!peek_file(input_root,
                       action.extract_ozz_animation().input_file_path())) {
          core::Logger::err(kLogLabel)
              << "Assimp file not found: "
              << input_root /
                     action.assimp_to_static_draco_geo().input_file_path();
          return false;
        }
        break;
      case pb::SingleAction::kExtractOzzSkeleton:
        if (!peek_file(input_root,
                       action.extract_ozz_skeleton().input_file_path())) {
          core::Logger::err(kLogLabel)
              << "Assimp file not found: "
              << input_root /
                     action.assimp_to_static_draco_geo().input_file_path();
          return false;
        }
        break;
      case pb::SingleAction::kExtractSkinnedDracoGeo:
        if (!peek_file(input_root,
                       action.extract_skinned_draco_geo().input_file_path())) {
          core::Logger::err(kLogLabel)
              << "Assimp file not found: "
              << input_root /
                     action.assimp_to_static_draco_geo().input_file_path();
          return false;
        }
        break;
      default:
        core::Logger::err(kLogLabel)
            << "Unexpected request_case for validate_inputs_exist";
        return false;
    }
  }

  return true;
}

bool PlanExecutor::peek_file(std::filesystem::path input_root,
                             std::string file_name) const {
  return std::filesystem::exists(input_root / file_name);
}

bool PlanExecutor::copy_wgsl_source(asset::pb::AssetPack& output_asset_pack,
                                    const pb::CopyWgslSourceAction& action,
                                    std::filesystem::path input_root,
                                    FileCache& file_cache) {
  return wgsl_processor_.copy_wgsl_source(
      output_asset_pack, action,
      file_cache.load_file(action.input_file_path()));
}

bool PlanExecutor::convert_assimp_file(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssimpToStaticDracoGeoAction& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  return assimp_geo_processor_.export_static_draco_geo(
      output_asset_pack, action, file_cache, assimp_scene_cache);
}

bool PlanExecutor::convert_skinned_assimp_file(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssimpExtractSkinnedMeshToDraco& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  return assimp_geo_processor_.export_skinned_draco_geo(
      output_asset_pack, action, file_cache, assimp_scene_cache);
}

bool PlanExecutor::assemble_navmesh(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssembleRecastNavMeshAction& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  return recast_navmesh_processor_.export_recast_navmesh(
      output_asset_pack, action, file_cache, assimp_scene_cache);
}

bool PlanExecutor::create_and_export_skeleton(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssimpExtractSkeletonToOzz& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  return assimp_animation_processor_.export_skeleton(
      output_asset_pack, action, file_cache, assimp_scene_cache);
}

bool PlanExecutor::create_and_export_animation(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssimpExtractAnimationToOzz& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  return assimp_animation_processor_.export_animation(
      output_asset_pack, action, file_cache, assimp_scene_cache);
}
