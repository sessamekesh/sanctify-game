#include <igcore/log.h>
#include <igplatform/file_promise.h>
#include <util/assimp_loader.h>

using namespace mapeditor;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "AssimpLoader";
}

std::shared_ptr<Promise<const aiScene*>> AssimpLoader::load_file(
    std::string file_name,
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list) {
  using PromiseT = Promise<const aiScene*>;

  auto it = scenes_.find(file_name);
  if (it != scenes_.end()) {
    const aiScene* f = it->second->scene;
    return PromiseT::immediate(std::move(f));
  }

  // Bug...
  if (currently_loading_.count(file_name) > 0) {
    return PromiseT::immediate(nullptr);
  }

  currently_loading_.insert(file_name);

  return FilePromise::Create(file_name, async_task_list)
      ->then<std::shared_ptr<AssimpSceneData>>(
          [file_name](const FilePromiseResultT& rsl)
              -> std::shared_ptr<AssimpSceneData> {
            if (rsl.is_right()) {
              Logger::err(kLogLabel)
                  << "Failed to load Assimp scene from " << file_name << ": "
                  << FilePromise::error_log_label(rsl.get_right());
              return nullptr;
            }

            std::shared_ptr<core::RawBuffer> raw_data = rsl.get_left();

            auto scene_data = std::make_shared<AssimpSceneData>();
            scene_data->importer = std::make_shared<Assimp::Importer>();
            const uint32_t import_flags =
                aiProcessPreset_TargetRealtime_Quality | aiProcess_Triangulate;

            scene_data->scene = scene_data->importer->ReadFileFromMemory(
                raw_data->get(), raw_data->size(), import_flags);
            if (!scene_data->scene) {
              Logger::err(kLogLabel)
                  << "Failed to parse Assimp file " << file_name << ": "
                  << scene_data->importer->GetErrorString();
              return nullptr;
            }

            return scene_data;
          },
          async_task_list)
      ->then<const aiScene*>(
          [this, file_name](
              const std::shared_ptr<AssimpSceneData>& data) -> const aiScene* {
            this->currently_loading_.erase(file_name);
            if (data == nullptr) {
              return nullptr;
            }
            this->scenes_.emplace(file_name, data);
            return data->scene;
          },
          main_thread_task_list);
}

std::shared_ptr<Promise<std::vector<std::string>>> AssimpLoader::get_mesh_names(
    std::string file_name,
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list) {
  return load_file(file_name, async_task_list, main_thread_task_list)
      ->then<std::vector<std::string>>(
          [this,
           file_name](const aiScene* const& scene) -> std::vector<std::string> {
            return this->mesh_names_in_loaded_file(file_name);
          },
          main_thread_task_list);
}

std::vector<std::string> AssimpLoader::file_names() {
  std::set<std::string> names;

  for (const auto& name : currently_loading_) {
    names.insert(name);
  }

  for (const auto& scene : scenes_) {
    names.insert(scene.first);
  }

  std::vector<std::string> tr;
  for (const auto& n : names) {
    tr.push_back(n);
  }

  return tr;
}

bool AssimpLoader::is_loading(std::string file_name) {
  return currently_loading_.count(file_name) > 0;
}

std::vector<std::string> AssimpLoader::mesh_names_in_loaded_file(
    std::string loaded_file_name) {
  auto it = scenes_.find(loaded_file_name);
  if (it == scenes_.end()) {
    return std::vector<std::string>();
  }

  auto scene = it->second->scene;

  std::vector<std::string> mesh_names;

  if (scene == nullptr) {
    return mesh_names;
  }

  for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    // Evil hack - not sure why I'm doing this...
    if (scene->mMeshes[i]->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
      continue;
    }
    mesh_names.push_back(scene->mMeshes[i]->mName.C_Str());
  }

  return mesh_names;
}

const aiMesh* AssimpLoader::get_loaded_mesh(std::string file_name,
                                            std::string mesh_name) const {
  auto it = scenes_.find(file_name);
  if (it == scenes_.end()) {
    return nullptr;
  }

  const auto* scene = it->second->scene;
  for (int i = 0; i < scene->mNumMeshes; i++) {
    // Evil hack - not sure why I'm doing this...
    if (scene->mMeshes[i]->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
      continue;
    }
    if (scene->mMeshes[i]->mName.C_Str() == mesh_name) {
      return scene->mMeshes[i];
    }
  }

  return nullptr;
}
