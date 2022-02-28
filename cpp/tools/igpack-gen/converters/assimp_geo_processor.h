#ifndef TOOLS_IGPACK_GEN_CONVERTERS_ASSIMP_GEO_PROCESSOR_H
#define TOOLS_IGPACK_GEN_CONVERTERS_ASSIMP_GEO_PROCESSOR_H

#include <igasset/proto/igasset.pb.h>
#include <igpack-gen/proto/igpack-plan.pb.h>
#include <util/assimp_scene_cache.h>
#include <util/file_cache.h>

#include <string>

namespace indigo::igpackgen {

class AssimpGeoProcessor {
 public:
  bool export_static_draco_geo(asset::pb::AssetPack& output_asset_pack,
                               const pb::AssimpToStaticDracoGeoAction& action,
                               FileCache& file_cache,
                               AssimpSceneCache& assimp_scene_cache);

  bool export_skinned_draco_geo(
      asset::pb::AssetPack& output_asset_pack,
      const pb::AssimpExtractSkinnedMeshToDraco& action, FileCache& file_cache,
      AssimpSceneCache& assimp_scene_cache);
};

}  // namespace indigo::igpackgen

#endif
