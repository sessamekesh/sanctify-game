#include <imgui.h>
#include <nfd.h>
#include <views/navmesh_params_view.h>

using namespace mapeditor;

NavMeshParamsView::NavMeshParamsView(
    std::shared_ptr<RecastParams> recast_params,
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list)
    : recast_params_(recast_params),
      cell_size_(recast_params->cell_size()),
      assimp_loader_(std::make_shared<AssimpLoader>()),
      async_task_list_(async_task_list),
      main_thread_task_list_(main_thread_task_list) {}

void NavMeshParamsView::render() {
  bool has_difference = false;
  bool is_all_valid = true;

  std::vector<std::string> assimp_file_names = assimp_loader_->file_names();
  if (ImGui::TreeNode("Loaded Assimp files")) {
    if (ImGui::Button("Add Assimp File")) {
      nfdchar_t* out_path = nullptr;
      nfdresult_t result = NFD_OpenDialog("fbx", nullptr, &out_path);

      if (result == NFD_OKAY) {
        assimp_loader_->load_file(out_path, async_task_list_,
                                  main_thread_task_list_);

        free(out_path);
      }
    }

    for (const auto& file_name : assimp_file_names) {
      if (assimp_loader_->is_loading(file_name)) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 85, 85, 255));
      } else {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(215, 255, 255, 255));
      }

      std::string full_key = file_name + "##assimp_file";
      if (ImGui::TreeNode(full_key.c_str())) {
        std::vector<std::string> mesh_names =
            assimp_loader_->mesh_names_in_loaded_file(file_name);

        for (const auto& name : mesh_names) {
          ImGui::Text(name.c_str());
        }

        ImGui::TreePop();
      }

      ImGui::PopStyleColor();
    }

    ImGui::TreePop();
  }

  ImGui::InputFloat("Cell Size", &cell_size_);
  if (cell_size_ != recast_params_->cell_size()) {
    has_difference = true;
  }

  // TODO (sessamekesh): continue here by expressing the list of RecastOps
  //  for an AssembleRecastNavMeshAction here. A new one can be added by
  //  selecting a type and filling in the required information before building.
  // Any incomplete/invalid sections should be highlighted in a light red color.

  if (has_difference) {
    if (is_all_valid) {
      if (ImGui::Button("Build!")) {
        recast_params_->cell_size(cell_size_);
      }
    } else {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.2f, 0.3f, 1.f));
      ImGui::Text("FIX VALIDATION ERRORS before building!");
      ImGui::PopStyleColor();
    }
  }
}
