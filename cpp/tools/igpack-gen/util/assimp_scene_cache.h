#ifndef TOOLS_IGPACK_GEN_UTIL_ASSIMP_SCENE_CACHE_H
#define TOOLS_IGPACK_GEN_UTIL_ASSIMP_SCENE_CACHE_H

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <igcore/maybe.h>
#include <util/file_cache.h>

#include <assimp/Importer.hpp>
#include <map>

namespace indigo::igpackgen {

struct AssimpSceneData {
  std::shared_ptr<Assimp::Importer> Importer;
  const aiScene* Scene;

  AssimpSceneData();
};

class AssimpSceneCache {
 public:
  AssimpSceneCache(uint32_t max_cache_size);

  core::Maybe<AssimpSceneData*> load_scene(FileCache& file_cache,
                                           std::string assimp_file_path);

  core::Maybe<aiMesh*> load_mesh(FileCache& file_cache,
                                 std::string assimp_file_path,
                                 std::string mesh_name);

 private:
  uint32_t max_cache_size_;
  uint32_t cache_size_;
  std::map<std::string, AssimpSceneData> loaded_scenes_;
  std::list<std::string> load_order_;
};

}  // namespace indigo::igpackgen

#endif
