#ifndef TOOLS_IGPACK_GEN_CONVERTERS_RECAST_NAVMESH_PROCESSOR_H
#define TOOLS_IGPACK_GEN_CONVERTERS_RECAST_NAVMESH_PROCESSOR_H

#include <igasset/proto/igasset.pb.h>
#include <ignav/detour_navmesh.h>
#include <ignav/recast_compiler.h>
#include <igpack-gen/proto/igpack-plan.pb.h>
#include <util/assimp_scene_cache.h>

namespace indigo::igpackgen {

class RecastNavmeshProcessor {
 public:
  bool export_recast_navmesh(asset::pb::AssetPack& output_asset_pack,
                             const pb::AssembleRecastNavMeshAction& action,
                             FileCache& file_cache,
                             AssimpSceneCache& assimp_scene_cache);
};

}  // namespace indigo::igpackgen

#endif
