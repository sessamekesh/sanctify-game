#ifndef LIBS_IGGPU_INCLUDE_IGGPU_PIPELINE_BUILDER_H
#define LIBS_IGGPU_INCLUDE_IGGPU_PIPELINE_BUILDER_H

#include <igasset/proto/igasset.pb.h>
#include <webgpu/webgpu_cpp.h>

#include <string>

namespace indigo::iggpu {

struct PipelineBuilder {
  static PipelineBuilder Create(const wgpu::Device& device,
                                const indigo::asset::pb::WgslSource& vs_src,
                                const indigo::asset::pb::WgslSource& fs_src);

  wgpu::ShaderModule vertModule;
  wgpu::ShaderModule fragModule;

  std::string vsEntryPoint;
  std::string fsEntryPoint;
};

}  // namespace indigo::iggpu

#endif
