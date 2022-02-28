#ifndef TOOLS_IGPACK_GEN_PLAN_EXECUTOR_H
#define TOOLS_IGPACK_GEN_PLAN_EXECUTOR_H

#include <converters/assimp_animation_processor.h>
#include <converters/assimp_geo_processor.h>
#include <converters/recast_navmesh_processor.h>
#include <converters/wgsl_processor.h>
#include <igasset/proto/igasset.pb.h>
#include <igpack-gen/proto/igpack-plan.pb.h>
#include <util/assimp_scene_cache.h>
#include <util/file_cache.h>

#include <filesystem>
#include <map>
#include <string>

namespace indigo::igpackgen {

struct PlanInvocationDesc {
  pb::IgpackGenPlan Plan;
  std::filesystem::path InputAssetPathRoot;
  std::filesystem::path OutputAssetPathRoot;
};

class PlanExecutor {
 public:
  PlanExecutor(uint32_t max_file_memory_cache);

  bool execute_plan(const PlanInvocationDesc& desc);

 private:
  bool validate_inputs_exist(std::filesystem::path input_root,
                             const pb::SingleIgpackPlan& plan) const;
  bool peek_file(std::filesystem::path input_root, std::string file_name) const;

 private:
  bool copy_wgsl_source(asset::pb::AssetPack& output_asset_pack,
                        const pb::CopyWgslSourceAction& action,
                        std::filesystem::path input_root,
                        FileCache& file_cache);

  bool convert_assimp_file(asset::pb::AssetPack& output_asset_pack,
                           const pb::AssimpToStaticDracoGeoAction& action,
                           FileCache& file_cache,
                           AssimpSceneCache& assimp_scene_cache);
  bool convert_skinned_assimp_file(
      asset::pb::AssetPack& output_asset_pack,
      const pb::AssimpExtractSkinnedMeshToDraco& action, FileCache& file_cache,
      AssimpSceneCache& assimp_scene_cache);
  bool assemble_navmesh(asset::pb::AssetPack& output_asset_pack,
                        const pb::AssembleRecastNavMeshAction& action,
                        FileCache& file_cache,
                        AssimpSceneCache& assimp_scene_cache);

  bool create_and_export_skeleton(asset::pb::AssetPack& output_asset_pack,
                                  const pb::AssimpExtractSkeletonToOzz& action,
                                  FileCache& file_cache,
                                  AssimpSceneCache& assimp_scene_cache);

  bool create_and_export_animation(
      asset::pb::AssetPack& output_asset_pack,
      const pb::AssimpExtractAnimationToOzz& action, FileCache& file_cache,
      AssimpSceneCache& assimp_scene_cache);

 private:
  uint32_t max_file_memory_cache_;
  WgslProcessor wgsl_processor_;
  AssimpGeoProcessor assimp_geo_processor_;
  AssimpAnimationProcessor assimp_animation_processor_;
  RecastNavmeshProcessor recast_navmesh_processor_;
};
}  // namespace indigo::igpackgen

#endif
