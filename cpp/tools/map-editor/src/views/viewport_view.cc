#include <igasync/promise_combiner.h>
#include <igcore/pod_vector.h>
#include <iggpu/util.h>
#include <imgui.h>
#include <views/viewport_view.h>

using namespace indigo;
using namespace core;
using namespace asset;
using namespace mapeditor;

namespace {
const char* kLogLabel = "ViewportView";

const wgpu::TextureFormat kViewportFormat = wgpu::TextureFormat::RGBA8Unorm;

struct Common3dParamsData {
  glm::mat4 matView;
  glm::mat4 matProj;
};

struct GridParamsData {
  glm::vec3 gridlineColor;
};

}  // namespace

std::shared_ptr<ViewportView> ViewportView::Create(
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    std::shared_ptr<RecastParams> recast_params,
    std::shared_ptr<RecastBuilder> recast_builder, wgpu::Device device) {
  auto tr = std::shared_ptr<ViewportView>(
      new ViewportView(main_thread_task_list, async_task_list, recast_params,
                       recast_builder, device));

  tr->load_view();

  return tr;
}

ViewportView::ViewportView(
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    std::shared_ptr<RecastParams> recast_params,
    std::shared_ptr<RecastBuilder> recast_builder, wgpu::Device device)
    : main_thread_task_list_(main_thread_task_list),
      async_task_list_(async_task_list),
      cameraTarget(0.f, 0.f, 0.f),
      current_viewport_width_(32),
      current_viewport_height_(32),
      next_viewport_width_(32),
      next_viewport_height_(32),
      time_to_resize_viewport_(0.f),
      gridMinBb(recast_params->min_bb()),
      gridMaxBb(recast_params->max_bb()),
      gridCs(recast_params->cell_size()),
      cameraRadius(50.f),
      cameraTilt(glm::radians(30.f)),
      cameraSpin(glm::radians(45.f)),
      recast_params_(recast_params),
      recast_builder_(recast_builder),
      device_(device),
      gridPipeline(nullptr),
      gridCommon3dBindGroup(nullptr),
      gridParamsBindGroup(nullptr),
      common3dVertParams(nullptr),
      viewportOutTex({}),
      viewportOutView(nullptr),
      gridVertexBuffer(nullptr),
      numGridVertices(0) {}

void ViewportView::load_view() {
  create_static_resources();
  recreate_target_viewport();
  recreate_grid_buffers();

  IgpackLoader base_app("resources/base-app.igpack", async_task_list_);

  auto base_resources_combiner = PromiseCombiner::Create();

  auto grid_vs_src_key = base_resources_combiner->add(
      base_app.extract_wgsl_shader("vsGrid", async_task_list_),
      async_task_list_);
  auto grid_fs_src_key = base_resources_combiner->add(
      base_app.extract_wgsl_shader("fsGrid", async_task_list_),
      async_task_list_);

  auto base_resources_promise = base_resources_combiner->combine();

  base_resources_promise->on_success(
      [this, l = shared_from_this(), grid_vs_src_key,
       grid_fs_src_key](const PromiseCombiner::PromiseCombinerResult& rsl) {
        const auto& grid_vs_rsl = rsl.get(grid_vs_src_key);
        const auto& grid_fs_rsl = rsl.get(grid_fs_src_key);

        if (grid_vs_rsl.is_right() || grid_fs_rsl.is_right()) {
          Logger::err(kLogLabel)
              << "Failed to create pipeline - failed to load shader sources";
          return;
        }

        const pb::WgslSource& vs_src = grid_vs_rsl.get_left();
        const pb::WgslSource& fs_src = grid_fs_rsl.get_left();

        auto vs_module = iggpu::create_shader_module(
            device_, vs_src.shader_source(), "GridVs");
        auto fs_module = iggpu::create_shader_module(
            device_, fs_src.shader_source(), "GridFs");

        wgpu::ColorTargetState grid_target{};
        grid_target.format = ::kViewportFormat;

        wgpu::FragmentState grid_frag_state{};
        grid_frag_state.entryPoint = fs_src.entry_point().c_str();
        grid_frag_state.targetCount = 1;
        grid_frag_state.targets = &grid_target;
        grid_frag_state.module = fs_module;

        wgpu::VertexAttribute pos_attr{};
        pos_attr.format = wgpu::VertexFormat::Float32x3;
        pos_attr.offset = 0;
        pos_attr.shaderLocation = 0;
        wgpu::VertexBufferLayout vb_layout{};
        vb_layout.arrayStride = sizeof(float) * 3;
        vb_layout.attributeCount = 1;
        vb_layout.attributes = &pos_attr;
        vb_layout.stepMode = wgpu::VertexStepMode::Vertex;

        auto dss = iggpu::depth_stencil_state_standard();

        wgpu::RenderPipelineDescriptor grid_pipeline_desc{};
        grid_pipeline_desc.depthStencil = &dss;
        grid_pipeline_desc.fragment = &grid_frag_state;
        grid_pipeline_desc.vertex.module = vs_module;
        grid_pipeline_desc.vertex.entryPoint = vs_src.entry_point().c_str();
        grid_pipeline_desc.vertex.bufferCount = 1;
        grid_pipeline_desc.vertex.buffers = &vb_layout;
        grid_pipeline_desc.label = "GridPipeline";
        grid_pipeline_desc.primitive.cullMode = wgpu::CullMode::None;
        grid_pipeline_desc.primitive.topology =
            wgpu::PrimitiveTopology::LineList;

        gridPipeline = device_.CreateRenderPipeline(&grid_pipeline_desc);

        auto common_bind_group_layout = gridPipeline.GetBindGroupLayout(0);
        auto grid_params_bind_group_layout = gridPipeline.GetBindGroupLayout(1);

        Vector<wgpu::BindGroupEntry> common_entries(1);
        common_entries.push_back(iggpu::buffer_bind_group_entry(
            0, common3dVertParams, sizeof(::Common3dParamsData)));
        wgpu::BindGroupDescriptor common_desc = iggpu::bind_group_desc(
            common_entries, common_bind_group_layout, "GridCommon3dBindGroup");
        gridCommon3dBindGroup = device_.CreateBindGroup(&common_desc);

        Vector<wgpu::BindGroupEntry> grid_params_entries(1);
        grid_params_entries.push_back(iggpu::buffer_bind_group_entry(
            0, gridParamsBuffer, sizeof(::GridParamsData)));
        wgpu::BindGroupDescriptor grid_params_desc = iggpu::bind_group_desc(
            grid_params_entries, grid_params_bind_group_layout,
            "GridParamsBindGroup");
        gridParamsBindGroup = device_.CreateBindGroup(&grid_params_desc);
      },
      main_thread_task_list_);
}

void ViewportView::update(float dt) { time_to_resize_viewport_ -= dt; }

void ViewportView::render(uint32_t w, uint32_t h) {
  if (w != next_viewport_width_ || h != next_viewport_height_) {
    next_viewport_width_ = w;
    next_viewport_height_ = h;
    time_to_resize_viewport_ = 0.1f;
  }

  if ((next_viewport_width_ != current_viewport_width_ ||
       next_viewport_height_ != current_viewport_height_) &&
      time_to_resize_viewport_ < 0.f) {
    current_viewport_width_ = next_viewport_width_;
    current_viewport_height_ = next_viewport_height_;
    recreate_target_viewport();
  }

  if (gridMinBb != recast_params_->min_bb() ||
      gridMaxBb != recast_params_->max_bb() ||
      gridCs != recast_params_->cell_size()) {
    gridMinBb = recast_params_->min_bb();
    gridMaxBb = recast_params_->max_bb();
    gridCs = recast_params_->cell_size();
    recreate_grid_buffers();
  }

  wgpu::RenderPassColorAttachment attachment{};
  if (recast_builder_->is_valid_state()) {
    attachment.clearColor = {0.55f, 0.65f, 0.55f, 1.f};
  } else {
    attachment.clearColor = {0.85f, 0.2f, 0.2f, 1.f};
  }
  attachment.loadOp = wgpu::LoadOp::Clear;
  attachment.storeOp = wgpu::StoreOp::Store;
  attachment.view = viewportOutView;

  wgpu::RenderPassDepthStencilAttachment depth_attachment{};
  depth_attachment.clearDepth = 1.f;
  depth_attachment.clearStencil = 0x00;
  depth_attachment.depthLoadOp = wgpu::LoadOp::Clear;
  depth_attachment.depthStoreOp = wgpu::StoreOp::Store;
  depth_attachment.stencilLoadOp = wgpu::LoadOp::Clear;
  depth_attachment.stencilStoreOp = wgpu::StoreOp::Discard;
  depth_attachment.view = depthView;

  wgpu::RenderPassDescriptor rp_desc{};
  rp_desc.colorAttachmentCount = 1;
  rp_desc.colorAttachments = &attachment;
  rp_desc.depthStencilAttachment = &depth_attachment;
  rp_desc.label = "PreviewViewportRenderPass";

  //
  // Update bind groups
  //
  glm::vec3 camera_pos =
      cameraTarget + glm::vec3(glm::sin(cameraTilt) * glm::sin(cameraSpin),
                               glm::cos(cameraTilt),
                               glm::sin(cameraTilt) * -glm::cos(cameraSpin)) *
                         cameraRadius;
  glm::mat4 mat_view =
      glm::lookAt(camera_pos, cameraTarget, glm::vec3(0.f, 1.f, 0.f));
  glm::mat4 mat_proj = glm::perspective(
      glm::radians(40.f),
      (float)current_viewport_width_ / (float)current_viewport_height_, 0.1f,
      1000.f);
  {
    ::Common3dParamsData commonData{};
    commonData.matView = mat_view;
    commonData.matProj = mat_proj;
    device_.GetQueue().WriteBuffer(common3dVertParams, 0, &commonData,
                                   sizeof(commonData));
  }

  wgpu::CommandEncoder vp_encoder = device_.CreateCommandEncoder();
  wgpu::RenderPassEncoder pass = vp_encoder.BeginRenderPass(&rp_desc);

  if (gridPipeline != nullptr && gridCommon3dBindGroup != nullptr &&
      gridParamsBindGroup != nullptr && numGridVertices > 0) {
    pass.SetPipeline(gridPipeline);
    pass.SetVertexBuffer(0, gridVertexBuffer);
    pass.SetBindGroup(0, gridCommon3dBindGroup);
    pass.SetBindGroup(1, gridParamsBindGroup);
    pass.Draw(numGridVertices);
  }

  pass.End();
  wgpu::CommandBuffer commands = vp_encoder.Finish();

  device_.GetQueue().Submit(1, &commands);

  //
  // ImGui submission
  //
  ImGui::Image((void*)viewportOutView.Get(), ImVec2(w, h));

  //
  // Let the recast builder at it if it would lik
  //
  recast_builder_->render(mat_view, mat_proj, camera_pos, viewportOutView,
                          depthView);
}

void ViewportView::create_static_resources() {
  common3dVertParams = iggpu::create_empty_buffer(
      device_, sizeof(::Common3dParamsData), wgpu::BufferUsage::Uniform);
}

void ViewportView::recreate_target_viewport() {
  Logger::log(kLogLabel) << "Resizing viewport out texture to "
                         << current_viewport_width_ << "x"
                         << current_viewport_height_;
  viewportOutTex = iggpu::create_empty_texture_2d(
      device_, current_viewport_width_, current_viewport_height_, 1,
      ::kViewportFormat,
      wgpu::TextureUsage::RenderAttachment |
          wgpu::TextureUsage::TextureBinding);
  auto view_desc = iggpu::view_desc_of(viewportOutTex);
  viewportOutView = viewportOutTex.GpuTexture.CreateView(&view_desc);

  depthTex = iggpu::create_empty_texture_2d(
      device_, current_viewport_width_, current_viewport_height_, 1,
      wgpu::TextureFormat::Depth24PlusStencil8,
      wgpu::TextureUsage::RenderAttachment);
  auto depth_view_desc = iggpu::view_desc_of(depthTex);
  depthView = depthTex.GpuTexture.CreateView(&depth_view_desc);
}

void ViewportView::recreate_grid_buffers() {
  PodVector<glm::vec3> lineVerts(256);

  // First: a box around the entire bounding area
  lineVerts.push_back({gridMinBb.x, gridMinBb.y, gridMinBb.z});
  lineVerts.push_back({gridMinBb.x, gridMinBb.y, gridMaxBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMinBb.y, gridMinBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMinBb.y, gridMaxBb.z});
  lineVerts.push_back({gridMinBb.x, gridMaxBb.y, gridMinBb.z});
  lineVerts.push_back({gridMinBb.x, gridMaxBb.y, gridMaxBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMaxBb.y, gridMinBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMaxBb.y, gridMaxBb.z});

  lineVerts.push_back({gridMinBb.x, gridMinBb.y, gridMinBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMinBb.y, gridMinBb.z});
  lineVerts.push_back({gridMinBb.x, gridMaxBb.y, gridMinBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMaxBb.y, gridMinBb.z});
  lineVerts.push_back({gridMinBb.x, gridMinBb.y, gridMaxBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMinBb.y, gridMaxBb.z});
  lineVerts.push_back({gridMinBb.x, gridMaxBb.y, gridMaxBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMaxBb.y, gridMaxBb.z});

  lineVerts.push_back({gridMinBb.x, gridMinBb.y, gridMinBb.z});
  lineVerts.push_back({gridMinBb.x, gridMaxBb.y, gridMinBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMinBb.y, gridMinBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMaxBb.y, gridMinBb.z});
  lineVerts.push_back({gridMinBb.x, gridMinBb.y, gridMaxBb.z});
  lineVerts.push_back({gridMinBb.x, gridMaxBb.y, gridMaxBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMinBb.y, gridMaxBb.z});
  lineVerts.push_back({gridMaxBb.x, gridMaxBb.y, gridMaxBb.z});

  // Next: draw XZ plane edges
  lineVerts.push_back({gridMinBb.x, 0.f, gridMinBb.z});
  lineVerts.push_back({gridMinBb.x, 0.f, gridMaxBb.z});
  lineVerts.push_back({gridMaxBb.x, 0.f, gridMinBb.z});
  lineVerts.push_back({gridMaxBb.x, 0.f, gridMaxBb.z});
  lineVerts.push_back({gridMinBb.x, 0.f, gridMinBb.z});
  lineVerts.push_back({gridMaxBb.x, 0.f, gridMinBb.z});
  lineVerts.push_back({gridMinBb.x, 0.f, gridMaxBb.z});
  lineVerts.push_back({gridMaxBb.x, 0.f, gridMaxBb.z});

  // Next: draw 10x cs gridlines
  float cs = recast_params_->cell_size();
  for (float x = gridMinBb.x; x < gridMaxBb.x; x += cs * 10.f) {
    lineVerts.push_back({x, 0.f, gridMinBb.z});
    lineVerts.push_back({x, 0.f, gridMaxBb.z});
  }
  for (float z = gridMinBb.z; z < gridMaxBb.z; z += cs * 10.f) {
    lineVerts.push_back({gridMinBb.x, 0.f, z});
    lineVerts.push_back({gridMaxBb.z, 0.f, z});
  }

  gridVertexBuffer =
      iggpu::buffer_from_data(device_, lineVerts, wgpu::BufferUsage::Vertex);
  numGridVertices = lineVerts.size();

  ::GridParamsData grid_params{};
  grid_params.gridlineColor = glm::vec3(0.85f, 0.85f, 0.85f);
  gridParamsBuffer =
      iggpu::buffer_from_data(device_, grid_params, wgpu::BufferUsage::Uniform);
}

float* ViewportView::camera_spin() { return &cameraSpin; }

float* ViewportView::camera_tilt() { return &cameraTilt; }

float* ViewportView::camera_look_at_x() { return &cameraTarget.x; }
float* ViewportView::camera_look_at_y() { return &cameraTarget.y; }
float* ViewportView::camera_look_at_z() { return &cameraTarget.z; }
float* ViewportView::camera_radius() { return &cameraRadius; }