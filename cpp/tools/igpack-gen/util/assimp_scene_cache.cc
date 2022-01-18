#include <igcore/log.h>
#include <util/assimp_scene_cache.h>

using namespace indigo;
using namespace igpackgen;

namespace {
const char* kLogLabel = "AssimpSceneCache";
}

AssimpSceneData::AssimpSceneData() : Importer(nullptr), Scene(nullptr) {}

AssimpSceneCache::AssimpSceneCache(uint32_t max_cache_size)
    : max_cache_size_(max_cache_size), cache_size_(0u) {}

core::Maybe<AssimpSceneData*> AssimpSceneCache::load_scene(
    FileCache& file_cache, std::string file_name) {
  const std::string& raw_data = file_cache.load_file(file_name);

  if (raw_data.size() == 0u) {
    core::Logger::err(kLogLabel) << "Failed to load read file at " << file_name;
    return core::Maybe<AssimpSceneData*>::empty();
  }

  if (loaded_scenes_.count(file_name) > 0) {
    auto existing_it =
        std::find(load_order_.begin(), load_order_.end(), file_name);
    if (existing_it != load_order_.end()) {
      load_order_.erase(existing_it);
    }
    load_order_.push_back(file_name);
    return &loaded_scenes_[file_name];
  }

  AssimpSceneData data;
  data.Importer = std::make_shared<Assimp::Importer>();
  data.Importer->SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
  const uint32_t import_flags =
      aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs;

  data.Scene = data.Importer->ReadFileFromMemory(&raw_data[0], raw_data.size(),
                                                 import_flags);
  if (!data.Scene) {
    core::Logger::err(kLogLabel) << "Failed to parse Assimp file (" << file_name
                                 << "): " << data.Importer->GetErrorString();
    return core::Maybe<AssimpSceneData*>::empty();
  }

  aiMemoryInfo memory_usage;
  data.Importer->GetMemoryRequirements(memory_usage);

  while ((cache_size_ + memory_usage.total) > max_cache_size_ &&
         loaded_scenes_.size() > 0) {
    std::string file_to_delete = *load_order_.begin();
    load_order_.pop_front();

    auto scene_it = loaded_scenes_.find(file_to_delete);
    if (scene_it != loaded_scenes_.end()) {
      aiMemoryInfo deleted_memory_usage;
      scene_it->second.Importer->GetMemoryRequirements(deleted_memory_usage);
      cache_size_ -= deleted_memory_usage.total;
      loaded_scenes_.erase(scene_it);
    }
  }

  load_order_.push_front(file_name);
  loaded_scenes_.emplace(file_name, std::move(data));
  cache_size_ += memory_usage.total;

  return &loaded_scenes_[file_name];
}

core::Maybe<aiMesh*> AssimpSceneCache::load_mesh(FileCache& file_cache,
                                                 std::string assimp_file_path,
                                                 std::string mesh_name) {
  core::Maybe<AssimpSceneData*> maybe_data =
      load_scene(file_cache, assimp_file_path);

  if (maybe_data.is_empty()) {
    core::Logger::err(kLogLabel)
        << "No Assimp scene found at " << assimp_file_path;
    return core::Maybe<aiMesh*>::empty();
  }

  const aiScene* scene = maybe_data.get()->Scene;

  for (int i = 0; i < scene->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[i];

    if (mesh->mName.C_Str() == mesh_name) {
      return core::Maybe<aiMesh*>(mesh);
    }
  }

  core::Logger::err(kLogLabel)
      << "Mesh " << mesh_name << " not found in Assimp file "
      << assimp_file_path;
  return core::Maybe<aiMesh*>::empty();
}