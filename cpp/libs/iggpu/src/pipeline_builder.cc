#include <iggpu/pipeline_builder.h>
#include <iggpu/util.h>

using namespace indigo;
using namespace iggpu;

PipelineBuilder PipelineBuilder::Create(
    const wgpu::Device& device, const indigo::asset::pb::WgslSource& vs_src,
    const indigo::asset::pb::WgslSource& fs_src) {
  return PipelineBuilder{create_shader_module(device, vs_src.shader_source()),
                         create_shader_module(device, fs_src.shader_source()),
                         vs_src.entry_point(), fs_src.entry_point()};
}
