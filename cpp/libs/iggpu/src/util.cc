#include <igcore/log.h>
#include <iggpu/util.h>

using namespace indigo;
using namespace iggpu;

namespace {
#ifndef __EMSCRIPTEN__
class ShaderModuleChecker {
 public:
  ShaderModuleChecker(const char* name = "<<unnamed shader>>")
      : has_errors_(false),
        error_type_(BuildShaderError::CompileError),
        debug_name_(name) {}

  bool is_valid() const { return has_errors_; }

  void cb(wgpu::CompilationInfoRequestStatus status,
          wgpu::CompilationInfo const* info) {
    std::string status_str;
    switch (status) {
      case wgpu::CompilationInfoRequestStatus::Error:
        status_str = "Error";
        error_type_ = BuildShaderError::CompileError;
        break;
      case wgpu::CompilationInfoRequestStatus::DeviceLost:
        status_str = "DeviceLost";
        error_type_ = BuildShaderError::DeviceLost;
        break;
      default:
        status_str = "<<UNKNOWN>>";
    }

    if (status != wgpu::CompilationInfoRequestStatus::Success) {
      core::Logger::err(debug_name_) << "Compile failure type: " << status_str;
      has_errors_ = true;
    }

    for (int i = 0; i < info->messageCount; i++) {
      const auto& message = info->messages[i];

      if (message.type == wgpu::CompilationMessageType::Error) {
        core::Logger::err(debug_name_)
            << "Error - " << message.message << " (line " << message.lineNum
            << " pos " << message.linePos << ")";
        has_errors_ = true;
      } else {
        std::string status_str =
            (message.type == wgpu::CompilationMessageType::Info) ? "Info"
                                                                 : "Warning";
        core::Logger::log(debug_name_)
            << status_str << " - " << message.message << " (line "
            << message.lineNum << " pos " << message.linePos << ")";
      }
    }
  }

  BuildShaderError error_type() { return error_type_; }

  static void wgpu_compilation_info_callback_for_shader_module_checker(
      WGPUCompilationInfoRequestStatus status,
      WGPUCompilationInfo const* compilation_info, void* user_data) {
    auto* checker = reinterpret_cast<ShaderModuleChecker*>(user_data);

    checker->cb(
        static_cast<wgpu::CompilationInfoRequestStatus>(status),
        reinterpret_cast<wgpu::CompilationInfo const*>(compilation_info));

    delete checker;
  }

 private:
  bool has_errors_;
  BuildShaderError error_type_;
  const char* debug_name_;
};
#endif
}  // namespace

wgpu::ShaderModule iggpu::create_shader_module(const wgpu::Device& device,
                                               const std::string& wgsl_src,
                                               const char* debug_name) {
  wgpu::ShaderModuleWGSLDescriptor desc{};
  desc.source = &wgsl_src[0];
  desc.sType = wgpu::SType::ShaderModuleWGSLDescriptor;

  wgpu::ShaderModuleDescriptor sdesc{};
  sdesc.label = debug_name;
  sdesc.nextInChain = &desc;

  wgpu::ShaderModule shader_module = device.CreateShaderModule(&sdesc);

#ifndef __EMSCRIPTEN__
  // Do validation at least to give nice debug logging if something goes wrong
  auto checker = new ShaderModuleChecker(debug_name);
  shader_module.GetCompilationInfo(
      ShaderModuleChecker::
          wgpu_compilation_info_callback_for_shader_module_checker,
      (void*)checker);
#endif

  return shader_module;
}

wgpu::BindGroupDescriptor iggpu::bind_group_desc(
    const core::Vector<wgpu::BindGroupEntry>& entries,
    const wgpu::BindGroupLayout& layout, const char* debug_name) {
  wgpu::BindGroupDescriptor desc{};
  desc.entries = &entries[0];
  desc.entryCount = entries.size();
  desc.layout = layout;
  desc.label = debug_name;
  return desc;
}

wgpu::BindGroupEntry iggpu::buffer_bind_group_entry(uint32_t binding,
                                                    const wgpu::Buffer& buffer,
                                                    uint32_t size,
                                                    uint32_t offset) {
  wgpu::BindGroupEntry entry{};
  entry.binding = binding;
  entry.buffer = buffer;
  entry.size = size;
  entry.offset = offset;
  return entry;
}

wgpu::Buffer iggpu::create_empty_buffer(const wgpu::Device& device,
                                        uint32_t size,
                                        wgpu::BufferUsage usage) {
  wgpu::BufferDescriptor desc{};
  desc.size = size;
  desc.usage = usage | wgpu::BufferUsage::CopyDst;
  desc.mappedAtCreation = false;

  return device.CreateBuffer(&desc);
}