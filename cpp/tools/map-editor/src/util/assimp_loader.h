#ifndef TOOLS_MAP_EDITOR_SRC_UTIL_ASSIMP_LOADER_H
#define TOOLS_MAP_EDITOR_SRC_UTIL_ASSIMP_LOADER_H

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <igasync/promise.h>

#include <assimp/Importer.hpp>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace mapeditor {

class AssimpLoader {
 public:
  struct AssimpSceneData {
    std::shared_ptr<Assimp::Importer> importer;
    const aiScene* scene;

    AssimpSceneData() : importer(nullptr), scene(nullptr) {}
  };

 public:
  std::shared_ptr<indigo::core::Promise<const aiScene*>> load_file(
      std::string file_name,
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list);

  std::shared_ptr<indigo::core::Promise<std::vector<std::string>>>
  get_mesh_names(std::string file_name,
                 std::shared_ptr<indigo::core::TaskList> async_task_list,
                 std::shared_ptr<indigo::core::TaskList> main_thread_task_list);

  std::vector<std::string> file_names();
  bool is_loading(std::string file_name);
  std::vector<std::string> mesh_names_in_loaded_file(
      std::string loaded_file_name);

  const aiMesh* get_loaded_mesh(std::string file_name,
                                std::string mesh_name) const;

 private:
  std::set<std::string> currently_loading_;
  std::map<std::string, std::shared_ptr<AssimpSceneData>> scenes_;
};

}  // namespace mapeditor

#endif
