#include <igasset/igpack_loader.h>
#include <igasync/promise.h>
#include <igasync/promise_combiner.h>
#include <igcore/math.h>
#include <iggpu/util.h>
#include <igplatform/file_promise.h>
#include <util/recast_builder.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace mapeditor;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "RecastBuilder";

const int kUniformBufferSize = 256;

const LightingParams kDefaultLightingParams = {
    glm::normalize(glm::vec3(1.f, -5.f, 1.2f)), 0.4f, glm::vec3(1.f, 1.f, 1.f),
    20.f};

wgpu::RenderPipeline create_map_pipeline(const wgpu::Device& device,
                                         const asset::pb::WgslSource& vs_src,
                                         const asset::pb::WgslSource& fs_src) {
  auto vs_module = iggpu::create_shader_module(device, vs_src.shader_source(),
                                               "MapPreviewVs");
  auto fs_module = iggpu::create_shader_module(device, fs_src.shader_source(),
                                               "MapPreviewFs");

  wgpu::ColorTargetState target{};
  target.format = wgpu::TextureFormat::RGBA8Unorm;

  wgpu::FragmentState frag_state{};
  frag_state.entryPoint = fs_src.entry_point().c_str();
  frag_state.targetCount = 1;
  frag_state.targets = &target;
  frag_state.module = fs_module;

  wgpu::VertexAttribute pos_attr{};
  pos_attr.format = wgpu::VertexFormat::Float32x3;
  pos_attr.offset = 0;
  pos_attr.shaderLocation = 0;
  wgpu::VertexAttribute normal_attr{};
  normal_attr.format = wgpu::VertexFormat::Float32x3;
  normal_attr.offset = 0;
  normal_attr.shaderLocation = 1;

  wgpu::VertexBufferLayout pos_layout{};
  pos_layout.arrayStride = sizeof(float) * 3;
  pos_layout.attributeCount = 1;
  pos_layout.attributes = &pos_attr;
  pos_layout.stepMode = wgpu::VertexStepMode::Vertex;

  wgpu::VertexBufferLayout normal_layout{};
  normal_layout.arrayStride = sizeof(float) * 3;
  normal_layout.attributeCount = 1;
  normal_layout.attributes = &normal_attr;
  normal_layout.stepMode = wgpu::VertexStepMode::Vertex;

  wgpu::VertexBufferLayout layouts[2] = {pos_layout, normal_layout};

  auto dss = iggpu::depth_stencil_state_standard();

  wgpu::RenderPipelineDescriptor pipeline_desc{};
  pipeline_desc.depthStencil = &dss;
  pipeline_desc.fragment = &frag_state;
  pipeline_desc.vertex.module = vs_module;
  pipeline_desc.vertex.entryPoint = vs_src.entry_point().c_str();
  pipeline_desc.vertex.bufferCount = 2;
  pipeline_desc.vertex.buffers = layouts;
  pipeline_desc.label = "MapPreviewPipeline";
  pipeline_desc.primitive.cullMode = wgpu::CullMode::Back;
  pipeline_desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;

  return device.CreateRenderPipeline(&pipeline_desc);
}

inline int bit(int a, int b) { return (a & (1 << b)) >> b; }

glm::vec4 tile_index_to_color(int i) {
  i = i + 1;

  int r = ::bit(i, 1) + ::bit(i, 3) * 2 + 1;
  int g = ::bit(i, 2) + ::bit(i, 4) * 2 + 1;
  int b = ::bit(i, 0) + ::bit(i, 5) * 2 + 1;

  r *= 63;
  g *= 63;
  b *= 63;

  return glm::vec4((float)r / 255.f, (float)g / 255.f, (float)b / 255.f, 1.f);
}

}  // namespace

RecastBuilder::RecastBuilder(wgpu::Device device)
    : device_(device),
      lighting_params_buffer_(iggpu::buffer_from_data(
          device, ::kDefaultLightingParams, wgpu::BufferUsage::Uniform)),
      camera_vert_params_buffer_(iggpu::create_empty_buffer(
          device, sizeof(VsCameraParams), wgpu::BufferUsage::Uniform)),
      camera_frag_params_buffer_(iggpu::create_empty_buffer(
          device, sizeof(FsCameraParams), wgpu::BufferUsage::Uniform)),
      model_transform_buffer_(iggpu::create_empty_buffer(
          device, ::kUniformBufferSize * 16, wgpu::BufferUsage::Uniform)),
      model_color_buffer_(iggpu::create_empty_buffer(
          device, ::kUniformBufferSize * 16, wgpu::BufferUsage::Uniform)),
      model_buffer_capacity_(16),
      next_model_offset_(0) {
  // This is a particularly evil hack that abuses the fact that native file
  // loading does just happen on whatever task list is asking for it...
  auto tl = std::make_shared<TaskList>();

  asset::IgpackLoader loader("resources/base-app.igpack", tl);

  auto combiner = PromiseCombiner::Create();

  auto vs_key =
      combiner->add(loader.extract_wgsl_shader("vsMapPreview", tl), tl);
  auto fs_key =
      combiner->add(loader.extract_wgsl_shader("fsMapPreview", tl), tl);

  combiner->combine()->on_success(
      [this, vs_key,
       fs_key](const PromiseCombiner::PromiseCombinerResult& rsl) {
        const auto& vs = rsl.get(vs_key);
        const auto& fs = rsl.get(fs_key);

        if (vs.is_right()) {
          Logger::err(kLogLabel)
              << "VS load error: " << asset::to_string(vs.get_right());
          return;
        }

        if (fs.is_right()) {
          Logger::err(kLogLabel)
              << "FS load error: " << asset::to_string(fs.get_right());
          return;
        }

        const auto& vs_src = vs.get_left();
        const auto& fs_src = fs.get_left();

        map_preview_pipeline_ = ::create_map_pipeline(device_, vs_src, fs_src);
      },
      tl);

  // This is the evil hack - since everything runs synchronously on the desktop
  //  platform, all the promises above will finish without requiring this to be
  //  an asynchronous method.
  while (tl->execute_next()) {
#ifdef __EMSCRIPTEN__
#error "Evil hack cannot be used on web. Refactor RecastBuilder to be async"
#endif
  }
}

bool RecastBuilder::is_valid_state() const {
  return map_preview_pipeline_ != nullptr &&
         lighting_params_buffer_ != nullptr &&
         camera_vert_params_buffer_ != nullptr &&
         camera_frag_params_buffer_ != nullptr &&
         model_transform_buffer_ != nullptr && model_color_buffer_ != nullptr;
}

void RecastBuilder::set_lighting_params(LightingParams lighting_params) {
  device_.GetQueue().WriteBuffer(lighting_params_buffer_, 0, &lighting_params,
                                 sizeof(lighting_params));
}

void RecastBuilder::rebuild_from(std::shared_ptr<RecastParams> params,
                                 std::shared_ptr<AssimpLoader> loader) {
  assimp_renderables_.clear();
  recast_renderables_.clear();
  next_model_offset_ = 0;

  if (map_preview_pipeline_ == nullptr) {
    // Cannot proceed without a map preview pipeline
    return;
  }

  auto ops = params->recast_ops();

  nav::RecastCompiler recast_compiler(
      params->cell_size(), params->cell_height(), params->max_slope_degrees(),
      (int)floorf(params->walkable_climb() / params->cell_height()),
      (int)ceilf(params->walkable_height() / params->cell_height()),
      params->agent_radius() / params->cell_size(),
      (int)floorf(params->min_region_area() /
                  (params->cell_size() * params->cell_size())),
      (int)ceilf(params->merge_region_area() /
                 (params->cell_size() * params->cell_size())),
      params->max_contour_error(),
      (int)ceilf(params->max_edge_length() / params->cell_size()),
      params->detail_sample_distance(), params->detail_max_error());

  for (const auto& op : ops) {
    if (op.has_include_assimp_geo()) {
      auto maybe_geo = extract_geo(op.include_assimp_geo(), params, loader);
      if (maybe_geo.is_empty()) {
        continue;
      }
      auto geo = maybe_geo.move();

      if (!add_include_geo_to_compiler(recast_compiler, geo)) {
        continue;
      }

      auto maybe_renderable =
          build_from_geo(glm::vec4(0.5f, 0.85f, 0.6f, 1.f), geo);
      if (maybe_renderable.has_value()) {
        assimp_renderables_.push_back(maybe_renderable.move());
      }
    } else if (op.has_exclude_assimp_geo()) {
      // TODO (sessamekesh): First, add this to the actual Recast builder

      auto maybe_geo = extract_geo(op.exclude_assimp_geo(), params, loader);
      if (maybe_geo.is_empty()) {
        continue;
      }
      auto geo = maybe_geo.move();

      auto maybe_renderable =
          build_from_geo(glm::vec4(0.865f, 0.45f, 0.6f, 1.f), geo);
      if (maybe_renderable.has_value()) {
        assimp_renderables_.push_back(maybe_renderable.move());
      }
    }
  }

  auto build_rsl = recast_compiler.build_recast();
  if (build_rsl.is_empty()) {
    Logger::err(kLogLabel) << "Failed to build usable navmesh!";
    return;
  }
  auto recast_raw = build_rsl.move();
  params->bb(recast_raw.minBb, recast_raw.maxBb);

  auto raw_detour_bytes_rsl = recast_compiler.build_raw(recast_raw);
  if (raw_detour_bytes_rsl.is_empty()) {
    Logger::err(kLogLabel)
        << "Failed to build Detour navmesh raw bytes from Recast data";
    return;
  }
  auto raw_detour_bytes = raw_detour_bytes_rsl.move();

  auto detour_navmesh_opt =
      nav::RecastCompiler::navmesh_from_raw(std::move(raw_detour_bytes));
  if (detour_navmesh_opt.is_empty()) {
    Logger::err(kLogLabel) << "Failed to build Detour navmesh from raw data";
    return;
  }
  auto detour_navmesh = detour_navmesh_opt.move();

  add_navmesh_renderable(detour_navmesh);
}

Maybe<RecastBuilder::GeoExtraction> RecastBuilder::extract_geo(
    const indigo::igpackgen::pb::AssembleRecastNavMeshAction_AssimpGeoDef& def,
    std::shared_ptr<RecastParams> params,
    std::shared_ptr<AssimpLoader> loader) {
  // Get Assimp geo (must already be loaded...)
  std::string assimpFileName =
      (params->asset_root() / def.assimp_file_name()).string();
  std::string meshName = def.assimp_mesh_name();
  const aiMesh* mesh = loader->get_loaded_mesh(assimpFileName, meshName);

  if (mesh == nullptr) {
    return empty_maybe{};
  }

  glm::vec3 pos(def.position().x(), def.position().y(), def.position().z());
  glm::vec3 scl(def.scale().x(), def.scale().y(), def.scale().z());
  if (!def.has_scale()) {
    scl = glm::vec3(1.f, 1.f, 1.f);
  }
  glm::vec3 rot_axis(def.rotation().x(), def.rotation().y(),
                     def.rotation().z());
  float rot_angle = def.rotation().angle();
  if (!def.has_rotation()) {
    rot_axis = glm::vec3(0.f, 1.f, 0.f);
    rot_angle = 0.f;
  }

  glm::mat4 T = igmath::transform_matrix(pos, rot_axis, rot_angle, scl);

  PodVector<glm::vec3> positions(mesh->mNumVertices);
  PodVector<glm::vec3> normals(mesh->mNumVertices);
  PodVector<uint32_t> indices(mesh->mNumFaces * 3);

  for (int i = 0; i < mesh->mNumVertices; i++) {
    glm::vec4 raw_pos = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                         mesh->mVertices[i].z, 1.f};
    glm::vec4 raw_normal = {mesh->mNormals[i].x, mesh->mNormals[i].y,
                            mesh->mNormals[i].z, 0.f};

    glm::vec4 pos = T * raw_pos;
    glm::vec4 normal = T * raw_normal;

    positions.push_back({pos.x, pos.y, pos.z});
    normals.push_back({normal.x, normal.y, normal.z});
  }

  for (int i = 0; i < mesh->mNumFaces; i++) {
    if (mesh->mFaces[i].mNumIndices != 3) {
      Logger::err(kLogLabel)
          << "Encountered face with " << mesh->mFaces[i].mNumIndices
          << " indices (expected 3) - face index " << i << " on mesh "
          << mesh->mName.C_Str();
      continue;
    }
    indices.push_back(mesh->mFaces[i].mIndices[0]);
    indices.push_back(mesh->mFaces[i].mIndices[1]);
    indices.push_back(mesh->mFaces[i].mIndices[2]);
  }

  return GeoExtraction{std::move(positions), std::move(normals),
                       std::move(indices)};
}

Maybe<Renderable> RecastBuilder::build_from_geo(
    glm::vec4 color, const RecastBuilder::GeoExtraction& geo) {
  wgpu::Buffer positionVertexBuffer = iggpu::buffer_from_data(
      device_, geo.positions, wgpu::BufferUsage::Vertex);
  wgpu::Buffer normalVertexBuffer =
      iggpu::buffer_from_data(device_, geo.normals, wgpu::BufferUsage::Vertex);
  wgpu::Buffer indexBuffer =
      iggpu::buffer_from_data(device_, geo.indices, wgpu::BufferUsage::Index);
  uint32_t numIndices = geo.indices.size();

  // TODO (sessamekesh): Not this - actually use the transform provided by the
  // declaration
  glm::mat4 worldTransform = glm::mat4(1.f);
  glm::vec4 materialColor = color;

  int modelOffset = next_model_offset_++;

  if (modelOffset >= model_buffer_capacity_) {
    Logger::err(kLogLabel) << "Buffer expanding capability not yet implemented "
                              "- failing to add renderable";
    return empty_maybe{};
  }

  device_.GetQueue().WriteBuffer(model_transform_buffer_,
                                 modelOffset * ::kUniformBufferSize,
                                 &worldTransform, sizeof(glm::mat4));
  device_.GetQueue().WriteBuffer(model_color_buffer_,
                                 modelOffset * ::kUniformBufferSize,
                                 &materialColor, sizeof(glm::vec4));

  auto bind_group_layout = map_preview_pipeline_.GetBindGroupLayout(0);
  Vector<wgpu::BindGroupEntry> bind_group_entries(5);
  bind_group_entries.push_back(iggpu::buffer_bind_group_entry(
      0, camera_vert_params_buffer_, sizeof(glm::mat4) * 2, 0u));
  bind_group_entries.push_back(iggpu::buffer_bind_group_entry(
      1, model_transform_buffer_, sizeof(glm::mat4),
      ::kUniformBufferSize * modelOffset));
  bind_group_entries.push_back(iggpu::buffer_bind_group_entry(
      2, lighting_params_buffer_, sizeof(LightingParams), 0u));
  bind_group_entries.push_back(
      iggpu::buffer_bind_group_entry(3, model_color_buffer_, sizeof(glm::vec4),
                                     ::kUniformBufferSize * modelOffset));
  bind_group_entries.push_back(iggpu::buffer_bind_group_entry(
      4, camera_frag_params_buffer_, sizeof(glm::vec3), 0u));

  auto bind_group_desc =
      iggpu::bind_group_desc(bind_group_entries, bind_group_layout);
  auto bind_group = device_.CreateBindGroup(&bind_group_desc);

  return Renderable{positionVertexBuffer, normalVertexBuffer, indexBuffer,
                    numIndices,           worldTransform,     materialColor,
                    modelOffset,          bind_group};
}

void RecastBuilder::render(glm::mat4 matView, glm::mat4 matProj,
                           glm::vec3 cameraPos, wgpu::TextureView target,
                           wgpu::TextureView depthBuffer,
                           bool render_assimp_geo, bool render_navmesh_geo) {
  if (!is_valid_state()) return;
  if (assimp_renderables_.size() == 0 && recast_renderables_.size() == 0)
    return;

  // Update camera parameters
  VsCameraParams vs_camera_params{
      matView,
      matProj,
  };
  FsCameraParams fs_camera_params{
      cameraPos,
  };
  device_.GetQueue().WriteBuffer(camera_vert_params_buffer_, 0,
                                 &vs_camera_params, sizeof(vs_camera_params));
  device_.GetQueue().WriteBuffer(camera_frag_params_buffer_, 0,
                                 &fs_camera_params, sizeof(fs_camera_params));

  // Setup render pass that can be used by this pipeline...
  wgpu::RenderPassColorAttachment color_attachment{};
  color_attachment.loadOp = wgpu::LoadOp::Load;
  color_attachment.storeOp = wgpu::StoreOp::Store;
  color_attachment.view = target;

  wgpu::RenderPassDepthStencilAttachment depth_attachment{};
  depth_attachment.depthLoadOp = wgpu::LoadOp::Load;
  depth_attachment.depthStoreOp = wgpu::StoreOp::Store;
  depth_attachment.stencilLoadOp = wgpu::LoadOp::Load;
  depth_attachment.stencilStoreOp = wgpu::StoreOp::Discard;
  depth_attachment.view = depthBuffer;

  wgpu::RenderPassDescriptor rp_desc{};
  rp_desc.colorAttachmentCount = 1;
  rp_desc.colorAttachments = &color_attachment;
  rp_desc.depthStencilAttachment = &depth_attachment;
  rp_desc.label = "RecastBuilderPass";

  wgpu::CommandEncoder rp_encoder = device_.CreateCommandEncoder();
  wgpu::RenderPassEncoder pass = rp_encoder.BeginRenderPass(&rp_desc);

  // Map preview pipeline stuff
  pass.SetPipeline(map_preview_pipeline_);
  if (render_assimp_geo) {
    for (int i = 0; i < assimp_renderables_.size(); i++) {
      const auto& renderable = assimp_renderables_[i];

      pass.SetVertexBuffer(0, renderable.PositionVertexBuffer);
      pass.SetVertexBuffer(1, renderable.NormalVertexBuffer);
      pass.SetBindGroup(0, renderable.bindGroup);
      pass.SetIndexBuffer(renderable.IndexBuffer, wgpu::IndexFormat::Uint32);
      pass.DrawIndexed(renderable.NumIndices);
    }
  }

  if (render_navmesh_geo) {
    for (int i = 0; i < recast_renderables_.size(); i++) {
      const auto& renderable = recast_renderables_[i];

      pass.SetVertexBuffer(0, renderable.PositionVertexBuffer);
      pass.SetVertexBuffer(1, renderable.NormalVertexBuffer);
      pass.SetBindGroup(0, renderable.bindGroup);
      pass.SetIndexBuffer(renderable.IndexBuffer, wgpu::IndexFormat::Uint32);
      pass.DrawIndexed(renderable.NumIndices);
    }
  }

  pass.End();
  wgpu::CommandBuffer commands = rp_encoder.Finish();

  device_.GetQueue().Submit(1, &commands);
}

bool RecastBuilder::add_include_geo_to_compiler(
    indigo::nav::RecastCompiler& compiler,
    const RecastBuilder::GeoExtraction& geo) {
  compiler.add_walkable_triangles(geo.positions, geo.indices);
  return true;
}

// See also:
// https://github.com/recastnavigation/recastnavigation/blob/c5cbd53024c8a9d8d097a4371215e3342d2fdc87/DebugUtils/Source/DetourDebugDraw.cpp#L120
void RecastBuilder::add_navmesh_renderable(
    const indigo::nav::DetourNavmesh& detour_navmesh) {
  for (int tile_index = 0; tile_index < detour_navmesh->getMaxTiles();
       tile_index++) {
    GeoExtraction tile_geo{};
    int next_index = 0;

    const dtMeshTile* tile = detour_navmesh->getTile(tile_index);
    if (tile == nullptr) break;

    dtPolyRef base = detour_navmesh->getPolyRefBase(tile);
    int tile_num = detour_navmesh->decodePolyIdTile(base);

    glm::vec4 tile_color = ::tile_index_to_color(tile_index);

    for (int i = 0; i < tile->header->polyCount; i++) {
      const dtPoly* p = &tile->polys[i];
      if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

      const dtPolyDetail* pd = &tile->detailMeshes[i];

      for (int j = 0; j < pd->triCount; j++) {
        const unsigned char* t = &tile->detailTris[(pd->triBase + j) * 4];

        glm::vec3 pos[3] = {};
        for (int k = 0; k < 3; k++) {
          float* vert_raw = nullptr;
          if (t[k] < p->vertCount) {
            vert_raw = &tile->verts[p->verts[t[k]] * 3];
          } else {
            vert_raw =
                &tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3];
          }
          pos[k] = glm::vec3(vert_raw[0], vert_raw[1], vert_raw[2]);
        }
        glm::vec3 normal =
            glm::normalize(glm::cross(pos[1] - pos[0], pos[2] - pos[0]));
        tile_geo.positions.push_back(pos[0]);
        tile_geo.positions.push_back(pos[1]);
        tile_geo.positions.push_back(pos[2]);
        tile_geo.normals.push_back(normal);
        tile_geo.normals.push_back(normal);
        tile_geo.normals.push_back(normal);
        tile_geo.indices.push_back(next_index++);
        tile_geo.indices.push_back(next_index++);
        tile_geo.indices.push_back(next_index++);
      }
    }

    auto maybe_renderable = build_from_geo(tile_color, tile_geo);
    if (maybe_renderable.is_empty()) {
      Logger::err(kLogLabel)
          << "Failed to build renderable from geo for tile #" << tile_index;
      continue;
    }

    recast_renderables_.push_back(maybe_renderable.move());
  }
}