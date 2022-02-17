#ifndef TOOLS_MAP_EDITOR_SRC_VIEWS_VIEWPORT_VIEW_H
#define TOOLS_MAP_EDITOR_SRC_VIEWS_VIEWPORT_VIEW_H

#include <igasset/igpack_loader.h>
#include <igasync/task_list.h>
#include <iggpu/texture.h>
#include <util/recast_builder.h>
#include <util/recast_params.h>
#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp>

namespace mapeditor {

// TODO (sessamekesh): Include the Assimp file loader doodad here
// TODO (sessamekesh): Include a RecastBuilder doodad here
//  RecastBuilder maintains all the vertices used in here, and is updated when
//  the RecastParams are rebuilt. RecastBuilder may be in a bad state if the
//  RecastParams are also in a bad state.

class ViewportView : public std::enable_shared_from_this<ViewportView> {
 public:
  static std::shared_ptr<ViewportView> Create(
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      std::shared_ptr<RecastParams> recast_params,
      std::shared_ptr<RecastBuilder> recast_builder, wgpu::Device device);

  void update(float dt);
  void render(uint32_t w, uint32_t h);

  float* camera_spin();
  float* camera_tilt();
  float* camera_look_at_x();
  float* camera_look_at_y();
  float* camera_look_at_z();
  float* camera_radius();

 private:
  ViewportView(std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
               std::shared_ptr<indigo::core::TaskList> async_task_list,
               std::shared_ptr<RecastParams> recast_params,
               std::shared_ptr<RecastBuilder> recast_builder,
               wgpu::Device device);
  void load_view();

  void create_static_resources();
  void recreate_grid_buffers();
  void recreate_target_viewport();

 private:
  //
  // Logistics
  //
  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;
  std::shared_ptr<indigo::core::TaskList> async_task_list_;

  //
  // Logical presentation data
  //
  glm::vec3 cameraTarget;
  float cameraRadius;
  float cameraTilt;
  float cameraSpin;
  uint32_t current_viewport_width_;
  uint32_t current_viewport_height_;
  uint32_t next_viewport_width_;
  uint32_t next_viewport_height_;
  float time_to_resize_viewport_;

  glm::vec3 gridMinBb;
  glm::vec3 gridMaxBb;
  float gridCs;

  //
  // Shared logical resources
  //
  std::shared_ptr<RecastParams> recast_params_;
  std::shared_ptr<RecastBuilder> recast_builder_;

  //
  // WebGPU render resources
  //
  wgpu::Device device_;

  wgpu::RenderPipeline gridPipeline;
  wgpu::BindGroup gridCommon3dBindGroup;
  wgpu::BindGroup gridParamsBindGroup;
  wgpu::Buffer common3dVertParams;
  wgpu::Buffer gridParamsBuffer;
  wgpu::Buffer gridVertexBuffer;
  uint32_t numGridVertices;

  indigo::iggpu::Texture viewportOutTex;
  wgpu::TextureView viewportOutView;

  indigo::iggpu::Texture depthTex;
  wgpu::TextureView depthView;
};

}  // namespace mapeditor

#endif
