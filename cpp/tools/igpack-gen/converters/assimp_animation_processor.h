#ifndef TOOLS_IGPACK_GEN_CONVERTERS_ASSIMP_ANIMATION_PROCESSOR_H
#define TOOLS_IGPACK_GEN_CONVERTERS_ASSIMP_ANIMATION_PROCESSOR_H

#include <igasset/proto/igasset.pb.h>
#include <igpack-gen/proto/igpack-plan.pb.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/memory/unique_ptr.h>
#include <util/assimp_scene_cache.h>
#include <util/file_cache.h>

#include <map>
#include <string>

namespace indigo::igpackgen {

class AssimpAnimationProcessor {
 private:
  struct SkeletonMetadata {
    std::set<std::string> boneNames;
    std::string rootName;
  };

 public:
  bool preload_animation_bones(const pb::AssimpExtractAnimationToOzz& action,
                               FileCache& file_cache,
                               AssimpSceneCache& assimp_scene_cache);

  bool validate_bones_exist(const pb::AssimpExtractSkeletonToOzz& action,
                            FileCache& file_cache,
                            AssimpSceneCache& assimp_scene_cache);

  bool export_skeleton(asset::pb::AssetPack& output_asset_pack,
                       const pb::AssimpExtractSkeletonToOzz& action,
                       FileCache& file_cache,
                       AssimpSceneCache& assimp_scene_cache);

  bool export_animation(asset::pb::AssetPack& output_asset_pack,
                        const pb::AssimpExtractAnimationToOzz& action,
                        FileCache& file_cache,
                        AssimpSceneCache& assimp_scene_cache);

 private:
  std::map<std::string, SkeletonMetadata> skeleton_bones_;

  std::map<std::string, ozz::unique_ptr<ozz::animation::Skeleton>>
      built_skeletons_;
};

}  // namespace indigo::igpackgen

#endif
