#include <google/protobuf/util/message_differencer.h>
#include <igcore/log.h>
#include <imgui.h>
#include <nfd.h>
#include <views/navmesh_params_view.h>

using namespace indigo;
using namespace core;
using namespace mapeditor;

namespace {
const char* kLogLabel = "NavMeshParamsView";
const size_t kFullProtoTextBufferSize = 32000;

// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
// Released CC0 1.0 by author
template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
               1;  // Extra space for '\0'
  if (size_s <= 0) {
    throw std::runtime_error("Error during formatting.");
  }
  auto size = static_cast<size_t>(size_s);
  auto buf = std::make_unique<char[]>(size);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(),
                     buf.get() + size - 1);  // We don't want the '\0' inside
}

}  // namespace

NavMeshParamsView::NavMeshParamsView(
    std::shared_ptr<RecastParams> recast_params,
    std::shared_ptr<RecastBuilder> recast_builder,
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list)
    : recast_params_(recast_params),
      recast_builder_(recast_builder),
      cell_size_(recast_params->cell_size()),
      recast_ops_(recast_params->recast_ops()),
      assimp_loader_(std::make_shared<AssimpLoader>()),
      async_task_list_(async_task_list),
      main_thread_task_list_(main_thread_task_list),
      is_valid_path_(false),
      full_proto_text_(new char[::kFullProtoTextBufferSize]),
      text_matches_(true),
      has_text_error_(false) {
  std::string asset_root = recast_params->asset_root().string();
  strcpy(asset_root_, asset_root.c_str());

  strcpy(full_proto_text_, recast_params->proto_text().c_str());
}

NavMeshParamsView::~NavMeshParamsView() { delete[] full_proto_text_; }

void NavMeshParamsView::render() {
  bool has_difference = false;
  std::vector<std::string> validation_errors;

  int delete_idx = -1;

  if (ImGui::InputText("Asset Root", asset_root_, 256)) {
    is_valid_path_ = std::filesystem::is_directory(asset_root_);
  }

  if (!is_valid_path_) {
    ImGui::Text("Invalid root directory");
  } else {
    std::string asset_root = recast_params_->asset_root().string();
    std::string gui_root = asset_root_;
    if (asset_root != gui_root) {
      if (ImGui::Button("Update Asset Root")) {
        recast_params_->asset_root(gui_root);
      }
    }
  }

  if (ImGui::TreeNode("Loaded Assimp files")) {
    std::vector<std::string> assimp_file_names = assimp_loader_->file_names();
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
  if (ImGui::TreeNode("Recast Build Ops")) {
    if (ImGui::Button("Add build op")) {
      recast_ops_.push_back(
          indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp{});
    }

    auto stored_build_ops = recast_params_->recast_ops();

    if (stored_build_ops.size() != recast_ops_.size()) {
      has_difference = true;
    }

    for (int i = 0; i < recast_ops_.size(); i++) {
      auto& view_op = recast_ops_[i];
      if (stored_build_ops.size() <= i) {
        has_difference = true;
      } else if (!google::protobuf::util::MessageDifferencer::Equals(
                     view_op, stored_build_ops[i])) {
        has_difference = true;
      }

      if (view_op.op_case() ==
          indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp::OpCase::
              OP_NOT_SET) {
        view_op.mutable_include_assimp_geo();
      }

      //
      // Label of resource - change ordering in here also
      //
      std::string node_label = "";
      switch (view_op.op_case()) {
        case indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp::
            OpCase::kIncludeAssimpGeo:
          node_label = ::string_format("(%i) - Include Assimp Geo", i);
          break;
        case indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp::
            OpCase::kExcludeAssimpGeo:
          node_label = ::string_format("(%i) - Exclude Assimp Geo", i);
          break;
        default:
          validation_errors.push_back(::string_format(
              "Operation %i has invalid type - check proto", i));
          node_label = ::string_format("(%i) <<INVALID>>", i);
      }

      if (ImGui::TreeNode(node_label.c_str())) {
        ImGui::Text("...");
        if (i > 0) {
          ImGui::SameLine();
          if (ImGui::Button(::string_format("^##%i", i).c_str())) {
            indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp tmp =
                recast_ops_[i];
            recast_ops_[i] = recast_ops_[i - 1];
            recast_ops_[i - 1] = tmp;
          }
        }
        if (i < (recast_ops_.size() - 1)) {
          ImGui::SameLine();
          if (ImGui::Button(::string_format("v##%i", i).c_str())) {
            indigo::igpackgen::pb::AssembleRecastNavMeshAction_RecastOp tmp =
                recast_ops_[i];
            recast_ops_[i] = recast_ops_[i + 1];
            recast_ops_[i + 1] = tmp;
            Logger::log(kLogLabel) << "Moving up " << i;
          }
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 10, 20, 255));
        if (ImGui::Button(::string_format("X##%i", i).c_str())) {
          delete_idx = i;
        }
        ImGui::PopStyleColor();

        //
        // Selectable type row - click button to change which type it is
        //
        {
          if (view_op.has_include_assimp_geo()) {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 150, 255));
            ImGui::Button(::string_format("Include Assimp Geo##%i", i).c_str());
            ImGui::PopStyleColor();
          } else {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(50, 50, 50, 255));
            if (ImGui::Button(
                    ::string_format("Include Assimp Geo##%i", i).c_str())) {
              view_op.mutable_include_assimp_geo();
            }
            ImGui::PopStyleColor();
          }

          ImGui::SameLine();

          if (view_op.has_exclude_assimp_geo()) {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 150, 255));
            ImGui::Button(::string_format("Exclude Assimp Geo##%i", i).c_str());
            ImGui::PopStyleColor();
          } else {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(50, 50, 50, 255));
            if (ImGui::Button(
                    ::string_format("Exclude Assimp Geo##%i", i).c_str())) {
              view_op.mutable_exclude_assimp_geo();
            }
            ImGui::PopStyleColor();
          }
        }

        //
        // Actual implementation of selectable
        //
        {
          if (view_op.has_include_assimp_geo()) {
            for (const auto& err : render_assimp_geo_def(
                     i, *view_op.mutable_include_assimp_geo())) {
              validation_errors.push_back(err);
            }
          }
          if (view_op.has_exclude_assimp_geo()) {
            for (const auto& err : render_assimp_geo_def(
                     i, *view_op.mutable_exclude_assimp_geo())) {
              validation_errors.push_back(err);
            }
          }
        }

        ImGui::Separator();
        ImGui::TreePop();
      }
    }

    ImGui::TreePop();
  }

  if (has_difference) {
    if (validation_errors.size() == 0) {
      if (ImGui::Button("Build!")) {
        recast_params_->cell_size(cell_size_);
        recast_params_->recast_ops(recast_ops_);

        sync_from_recast();
      }
    } else {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.2f, 0.3f, 1.f));
      for (int i = 0; i < validation_errors.size(); i++) {
        ImGui::Text(validation_errors[i].c_str());
      }
      ImGui::PopStyleColor();
    }
  }

  if (delete_idx >= 0 && delete_idx < recast_ops_.size()) {
    recast_ops_.erase(recast_ops_.begin() + delete_idx);
  }

  if (ImGui::TreeNode("Proto Text")) {
    if (ImGui::InputTextMultiline("##protosource", full_proto_text_,
                                  ::kFullProtoTextBufferSize)) {
      std::string t = full_proto_text_;
      text_matches_ = t == recast_params_->proto_text();
      has_text_error_ = false;
    }

    if (has_text_error_) {
      ImGui::TextColored(ImVec4(1.f, 0.1f, 0.1f, 1.f), "Invalid Proto");
    }

    if (!text_matches_) {
      if (ImGui::Button("Update Proto Text")) {
        std::string new_text = full_proto_text_;
        if (recast_params_->proto_text(new_text)) {
          sync_from_recast();
        } else {
          has_text_error_ = true;
          text_matches_ = false;
        }
      }
    }

    ImGui::TreePop();
  }
}

std::vector<std::string> NavMeshParamsView::render_assimp_geo_def(
    int i,
    indigo::igpackgen::pb::AssembleRecastNavMeshAction_AssimpGeoDef& geo_def) {
  std::vector<std::string> errs;

  if (ImGui::BeginListBox(::string_format("Assimp File List##%i", i).c_str())) {
    std::vector<std::string> file_names = assimp_loader_->file_names();

    for (const std::string& n : file_names) {
      std::error_code ec;
      std::string relative_path =
          std::filesystem::proximate(n, recast_params_->asset_root(), ec)
              .string();

      if (ec) {
        relative_path = n;
      }

      bool is_selected = geo_def.assimp_file_name() == relative_path;
      if (ImGui::Selectable(
              ::string_format("%s##%i", relative_path.c_str(), i).c_str(),
              is_selected)) {
        geo_def.set_assimp_file_name(relative_path);
      }
    }
    ImGui::EndListBox();
  }

  if (ImGui::BeginListBox(::string_format("Assimp Mesh List##%i", i).c_str())) {
    if (geo_def.assimp_file_name() == "") {
      ImGui::Selectable(::string_format("<<Empty>>##%i", i).c_str(), false);
    } else {
      std::string full_path =
          (std::filesystem::path(recast_params_->asset_root()) /
           geo_def.assimp_file_name())
              .string();

      std::vector<std::string> mesh_names =
          assimp_loader_->mesh_names_in_loaded_file(
              (recast_params_->asset_root() / geo_def.assimp_file_name())
                  .string());
      for (const std::string& n : mesh_names) {
        bool is_selected = geo_def.assimp_mesh_name() == n;
        if (ImGui::Selectable(::string_format("%s##%i", n.c_str(), i).c_str(),
                              is_selected)) {
          geo_def.set_assimp_mesh_name(n);
        }
      }
    }
    ImGui::EndListBox();
  }

  ImGui::Text(
      ::string_format("File name: %s", geo_def.assimp_file_name().c_str())
          .c_str());
  ImGui::Text(
      ::string_format("Mesh name: %s", geo_def.assimp_mesh_name().c_str())
          .c_str());

  if (geo_def.assimp_file_name() == "") {
    errs.push_back(::string_format("No Assimp file name on resource %i", i));
  }
  if (geo_def.assimp_mesh_name() == "") {
    errs.push_back(::string_format("No Assimp mesh name on resource %i", i));
  }

  return errs;
}

void NavMeshParamsView::sync_from_recast() {
  std::string ptt = recast_params_->proto_text();
  strcpy(full_proto_text_, ptt.c_str());
  text_matches_ = true;

  auto ops = recast_params_->recast_ops();
  for (const auto& op : ops) {
    if (op.has_include_assimp_geo()) {
      assimp_loader_->load_file((recast_params_->asset_root() /
                                 op.include_assimp_geo().assimp_file_name())
                                    .string(),
                                async_task_list_, main_thread_task_list_);
    }
    if (op.has_exclude_assimp_geo()) {
      assimp_loader_->load_file((recast_params_->asset_root() /
                                 op.include_assimp_geo().assimp_file_name())
                                    .string(),
                                async_task_list_, main_thread_task_list_);
    }
  }

  recast_ops_ = recast_params_->recast_ops();

  recast_builder_->rebuild_from(recast_params_, assimp_loader_);
}
