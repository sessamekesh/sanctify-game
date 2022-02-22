#include <igcore/log.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <map_editor_app.h>

namespace {
const char* kLogLabel = "MapEditorApp";
}

using namespace mapeditor;
using namespace indigo;
using namespace core;

std::shared_ptr<MapEditorApp> MapEditorApp::Create() {
  auto base_rsl = MapEditorAppWindow::Create();
  if (base_rsl.is_right()) {
    Logger::err(kLogLabel) << "Failed to create MapEditorAppWindow - error "
                           << to_string(base_rsl.get_right());
    return nullptr;
  }

  auto base = base_rsl.left_move();

  auto main_thread_task_list = std::make_shared<TaskList>();
  auto async_task_list = std::make_shared<TaskList>();

  Vector<std::shared_ptr<ExecutorThread>> executor_threads;
  for (int i = 1; i < std::thread::hardware_concurrency(); i++) {
    auto executor = std::make_shared<ExecutorThread>();
    executor->add_task_list(async_task_list);
    executor_threads.push_back(std::move(executor));
  }

  auto app = std::shared_ptr<MapEditorApp>(
      new MapEditorApp(base, main_thread_task_list, async_task_list,
                       std::move(executor_threads)));

  // TODO (sessamekesh): app initialization things here (async startup etc)
  app->setup_imgui();
  app->on_resize_swap_chain();

  return app;
}

MapEditorApp::MapEditorApp(
    std::shared_ptr<MapEditorAppWindow> base,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    indigo::core::Vector<std::shared_ptr<indigo::core::ExecutorThread>>
        executor_threads)
    : base_(base),
      main_thread_task_list_(main_thread_task_list),
      async_task_list_(async_task_list),
      next_swap_chain_width_(base->width()),
      next_swap_chain_height_(base->height()),
      time_to_next_swap_chain_update_(0.f),
      executor_threads_(std::move(executor_threads)),
      recast_params_(std::make_shared<RecastParams>()),
      recast_builder_(std::make_shared<RecastBuilder>(base->device())),
      viewport_view_(ViewportView::Create(main_thread_task_list,
                                          async_task_list, recast_params_,
                                          recast_builder_, base->device())),
      nav_mesh_params_view_(std::make_shared<NavMeshParamsView>(
          recast_params_, recast_builder_, async_task_list,
          main_thread_task_list)) {}

MapEditorApp::~MapEditorApp() { teardown_imgui(); }

void MapEditorApp::update(float dt) {
  time_to_next_swap_chain_update_ -= dt;
  viewport_view_->update(dt);
}

void MapEditorApp::render() {
  int vp_width = 0, vp_height = 0;
  glfwGetFramebufferSize(base_->window(), &vp_width, &vp_height);

  if (vp_width != next_swap_chain_width_ ||
      vp_height != next_swap_chain_height_) {
    next_swap_chain_width_ = vp_width;
    next_swap_chain_height_ = vp_height;
    time_to_next_swap_chain_update_ = 0.04f;
  }

  if ((next_swap_chain_width_ != base_->width() ||
       next_swap_chain_height_ != base_->height()) &&
      time_to_next_swap_chain_update_ < 0.f) {
    Logger::log(kLogLabel) << "Resizing swap chain from " << base_->width()
                           << "x" << base_->height() << " to "
                           << next_swap_chain_width_ << "x"
                           << next_swap_chain_height_;
    base_->resize_swap_chain(next_swap_chain_width_, next_swap_chain_height_);
    on_resize_swap_chain();
  }

  // ImGui setup
  ImGui_ImplGlfw_NewFrame();
  ImGui_ImplWGPU_NewFrame();
  ImGui::NewFrame();

  // ImGui declarations
  render_gui();

  // ImGui render submission
  ImGui::Render();

  wgpu::RenderPassColorAttachment attachment{};
  attachment.loadOp = wgpu::LoadOp::Clear;
  attachment.storeOp = wgpu::StoreOp::Store;
  attachment.clearColor = {0.5f, 0.5f, 0.5f, 1.f};
  attachment.view = base_->swap_chain().GetCurrentTextureView();

  wgpu::RenderPassDescriptor rp_desc{};
  rp_desc.colorAttachmentCount = 1;
  rp_desc.colorAttachments = &attachment;

  wgpu::CommandEncoder encoder = base_->device().CreateCommandEncoder();
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&rp_desc);

  ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
  pass.End();

  wgpu::CommandBuffer buffer = encoder.Finish();

  base_->device().GetQueue().Submit(1, &buffer);

  base_->swap_chain().Present();
}

void MapEditorApp::execute_tasks_until(
    std::chrono::high_resolution_clock::time_point end_time) {
  while (main_thread_task_list_->execute_next()) {
    if (std::chrono::high_resolution_clock::now() > end_time) {
      break;
    }
  }
}

bool MapEditorApp::should_quit() {
  return glfwWindowShouldClose(base_->window());
}

void MapEditorApp::on_resize_swap_chain() {
  ImGui_ImplWGPU_InvalidateDeviceObjects();
  ImGui_ImplWGPU_CreateDeviceObjects();
}

void MapEditorApp::render_gui() {
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoSavedSettings;

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);

  if (ImGui::Begin("Map Editor", nullptr, flags)) {
    // Left nav - generation state of the nav mesh, and included preview
    // geometry
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
      ImGui::BeginChild("LeftNavbarChild",
                        ImVec2(ImGui::GetContentRegionAvail().x * 0.25f,
                               ImGui::GetContentRegionAvail().y),
                        true);
      ImGui::PopStyleVar();

      nav_mesh_params_view_->render();

      ImGui::EndChild();
    }

    ImGui::SameLine();

    // Right half of screen
    {
      ImGui::BeginChild("RightSideOfStuff",
                        ImVec2(ImGui::GetContentRegionAvail().x,
                               ImGui::GetContentRegionAvail().y));

      // Center Viewport
      {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
        ImGui::BeginChild("RightNavbarChild",
                          ImVec2(ImGui::GetContentRegionAvail().x,
                                 ImGui::GetContentRegionAvail().y * 0.75f),
                          true);
        ImGui::PopStyleVar();

        viewport_view_->render(ImGui::GetContentRegionAvail().x,
                               ImGui::GetContentRegionAvail().y);

        ImGui::EndChild();
      }

      // Preview / test configuration window
      {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
        ImGui::BeginChild("BottomPreviewSettingsChild",
                          ImVec2(ImGui::GetContentRegionAvail().x,
                                 ImGui::GetContentRegionAvail().y),
                          true);
        ImGui::PopStyleVar();

        render_preview_settings_view();

        ImGui::EndChild();
      }

      ImGui::EndChild();
    }
  }
  ImGui::End();
}

void MapEditorApp::render_preview_settings_view() {
  ImGui::Text("Preview settings");

  if (ImGui::TreeNode("Camera Settings")) {
    ImGui::SliderAngle("Spin", viewport_view_->camera_spin());
    ImGui::SliderAngle("Tilt", viewport_view_->camera_tilt(), 0.0001f, 89.999f);
    ImGui::SliderFloat("Radius", viewport_view_->camera_radius(), 0.1f, 150.f,
                       "%.2f");
    ImGui::SliderFloat("Target X", viewport_view_->camera_look_at_x(),
                       recast_params_->min_bb().x, recast_params_->max_bb().x,
                       "%.2f");
    ImGui::SliderFloat("Target Y", viewport_view_->camera_look_at_y(),
                       recast_params_->min_bb().y, recast_params_->max_bb().y,
                       "%.2f");
    ImGui::SliderFloat("Target Z", viewport_view_->camera_look_at_z(),
                       recast_params_->min_bb().z, recast_params_->max_bb().z,
                       "%.2f");
    ImGui::Checkbox("Render map geometry", viewport_view_->render_map_geo());
    ImGui::Checkbox("Render navmesh geometry",
                    viewport_view_->render_navmesh_geo());
    ImGui::TreePop();
  }
}

void MapEditorApp::setup_imgui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.FontGlobalScale = base_->dpi();

  io.IniFilename = NULL;

  ImGui::StyleColorsDark();

  const wgpu::Device& device = base_->device();

  ImGui_ImplGlfw_InitForOther(base_->window(), true);
  ImGui_ImplWGPU_Init(device.Get(), 3,
                      (WGPUTextureFormat)base_->swap_chain_texture_format());
}

void MapEditorApp::teardown_imgui() {}