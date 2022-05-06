#include "simple_client_app_base.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>
#include <dawn/dawn_proc.h>
#include <dawn/native/DawnNative.h>
#include <igcore/config.h>
#include <igcore/log.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "SimpleClientAppBase.Native";

void print_wgpu_device_error(WGPUErrorType error_type, const char* message,
                             void*) {
  const char* error_type_name = "";
  switch (error_type) {
    case WGPUErrorType_Validation:
      error_type_name = "Validation";
      break;
    case WGPUErrorType_OutOfMemory:
      error_type_name = "OutOfMemory";
      break;
    case WGPUErrorType_DeviceLost:
      error_type_name = "DeviceLost";
      break;
    case WGPUErrorType_Unknown:
      error_type_name = "Unknown";
      break;
    default:
      error_type_name = "UNEXPECTED (this is bad)";
      break;
  }

  Logger::err("WGPU Device") << error_type_name << " error: " << message;
}

void device_lost_callback(WGPUDeviceLostReason reason, const char* msg,
                          void* user_data) {
  const char* lost_reason = "";
  switch (reason) {
    case WGPUDeviceLostReason_Destroyed:
      lost_reason = "WGPUDeviceLostReason_Destroyed";
    default:
      lost_reason = "WGPUDeviceLostReason_Unknown";
  }
  Logger::err("WGPU Device") << "Device lost (" << lost_reason << "): " << msg
                             << "\n   - Developer note: put a breakpoint here";
}

std::string backend_string(wgpu::BackendType type) {
  switch (type) {
    case wgpu::BackendType::D3D11:
      return "D3D11";
    case wgpu::BackendType::D3D12:
      return "D3D12";
    case wgpu::BackendType::Metal:
      return "Metal (eww)";
    case wgpu::BackendType::Null:
      return "Null (?)";
    case wgpu::BackendType::OpenGL:
      return "OpenGL";
    case wgpu::BackendType::OpenGLES:
      return "OpenGLES";
    case wgpu::BackendType::Vulkan:
      return "Vulkan";
    case wgpu::BackendType::WebGPU:
      return "WebGPU";
  }

  return "UnknownBackend";
}

std::string adapter_string(wgpu::AdapterType type) {
  switch (type) {
    case wgpu::AdapterType::CPU:
      return "CPU";
    case wgpu::AdapterType::DiscreteGPU:
      return "Discrete GPU";
    case wgpu::AdapterType::IntegratedGPU:
      return "Integrated GPU";
  }
}

dawn_native::Adapter get_adapter(
    const std::vector<dawn_native::Adapter>& adapters) {
  // Quickly list out all adapters
  {
    auto log = Logger::log(kLogLabel);
    log << "Enumerating adapters:\n";
    int i = 0;
    for (auto& adapter : adapters) {
      wgpu::AdapterProperties adapter_properties{};
      adapter.GetProperties(&adapter_properties);

      log << "(" << i++ << ") " << adapter_properties.name << " - "
          << adapter_properties.driverDescription << "\n"
          << "  Backend: " << ::backend_string(adapter_properties.backendType)
          << " (" << ::adapter_string(adapter_properties.adapterType) << ")\n";
    }
  }

  wgpu::AdapterType adapter_type_order[] = {
      wgpu::AdapterType::DiscreteGPU,
      wgpu::AdapterType::IntegratedGPU,
      wgpu::AdapterType::CPU,
  };

  wgpu::BackendType backend_type_order[] = {
#ifdef _WIN32
      // TODO (sessamekesh): Instead look for Dawn defines around if D3D12 is
      // supported etc.
      wgpu::BackendType::D3D12,
#endif
      wgpu::BackendType::Vulkan,
  };

  // Prefer discrete GPU, then integrated.
  // Between that, prefer whichever backend is best suited for the platform
  //  (D3D12 on Windows and then Vulkan, only Vulkan on Linux)
  for (int preferred_type_idx = 0;
       preferred_type_idx < std::size(adapter_type_order);
       preferred_type_idx++) {
    for (int i = 0; i < adapters.size(); i++) {
      const auto& adapter = adapters[i];
      wgpu::AdapterProperties properties{};
      adapter.GetProperties(&properties);

      for (int backend_type_idx = 0;
           backend_type_idx < std::size(backend_type_order);
           backend_type_idx++) {
        if (properties.backendType == backend_type_order[backend_type_idx] &&
            properties.adapterType == adapter_type_order[preferred_type_idx]) {
          Logger::log(kLogLabel) << "Using adapter #" << i;
          return adapter;
        }
      }
    }
  }

  // No suitable adapter found!
  Logger::err(kLogLabel) << "No suitable adapter found!";
  return {};
}

// Texture format generally preferred by desktop platforms, should always work
const wgpu::TextureFormat kPreferredTextureFormat =
    wgpu::TextureFormat::BGRA8Unorm;

}  // namespace

wgpu::TextureFormat SimpleClientAppBase::swap_chain_format() const {
  return ::kPreferredTextureFormat;
}

void SimpleClientAppBase::resize_swap_chain(uint32_t width, uint32_t height) {
#ifdef _WIN32
  WGPUSurfaceDescriptorFromWindowsHWND sdesc{};
  sdesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
  sdesc.hwnd = glfwGetWin32Window(window);
#else
  WGPUSurfaceDescriptorFromXlibWindow sdesc{};
  sdesc.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
  sdesc.display = glfwGetX11Display();
  sdesc.window = glfwGetX11Window(Window);
#endif

  WGPUSurfaceDescriptor desc{};
  desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&sdesc);

  wgpu::SwapChainDescriptor swap_chain_desc{};
  swap_chain_desc.format = ::kPreferredTextureFormat;
  swap_chain_desc.usage = wgpu::TextureUsage::RenderAttachment;
  swap_chain_desc.width = width;
  swap_chain_desc.height = height;
  swap_chain_desc.presentMode = wgpu::PresentMode::Fifo;  // TODO (sessamekesh)
  swapChain = device.CreateSwapChain(surface, &swap_chain_desc);
  width = width;
  height = height;

  if (swapChain == nullptr) {
    Logger::err(kLogLabel) << "Failed to recreate swap chain";
  }

  fire_swap_chain_resized_events(width, height);
}

glm::uvec2 SimpleClientAppBase::get_suggested_window_size() {
  int i_width = 0, i_height = 0;
  auto primary_monitor = glfwGetPrimaryMonitor();
  glfwGetMonitorWorkarea(primary_monitor, nullptr, nullptr, &i_width,
                         &i_height);

  if (i_width >= 1980 && i_height >= 1080) {
    return {1980u, 1080u};
  } else if (i_width >= 800 && i_height >= 600) {
    return {800u, 600u};
  } else {
    return {320u, 200u};
  }
}

Either<wgpu::Device, SimpleClientAppBase::CreateError>
SimpleClientAppBase::create_device() {
  auto instance = std::make_unique<dawn::native::Instance>();
  instance->DiscoverDefaultAdapters();

  dawn_native::Adapter adapter = ::get_adapter(instance->GetAdapters());
  if (!adapter) {
    return right(CreateError::WGPUNoSuitableAdapters);
  }

  // Feature toggles
  wgpu::DawnTogglesDeviceDescriptor feature_toggles{};
  std::vector<const char*> enabled_toggles;

  // Prevents accidental use of SPIR-V, since that isn't supported in
  //  web targets, allegedly because Apple is a piece of shit company that
  //  doesn't give a flying fuck about graphics developers.
  enabled_toggles.push_back("disallow_spirv");

#ifdef IG_ENABLE_GRAPHICS_DEBUGGING
  enabled_toggles.push_back("emit_hlsl_debug_symbols");
  enabled_toggles.push_back("disable_symbol_renaming");
#endif

  feature_toggles.forceEnabledTogglesCount = enabled_toggles.size();
  feature_toggles.forceEnabledToggles = &enabled_toggles[0];

  wgpu::DeviceDescriptor device_desc{};
  device_desc.nextInChain = &feature_toggles;
  device_desc.label = "DawnDevice";

  WGPUDevice raw_device = adapter.CreateDevice(&device_desc);
  if (!raw_device) {
    return right(CreateError::WGPUDeviceCreationFailed);
  }

  DawnProcTable procs = dawn_native::GetProcs();
  dawnProcSetProcs(&procs);

  procs.deviceSetUncapturedErrorCallback(raw_device, ::print_wgpu_device_error,
                                         nullptr);
  procs.deviceSetDeviceLostCallback(raw_device, ::device_lost_callback,
                                    nullptr);

  return left(wgpu::Device::Acquire(raw_device));
}

Either<SimpleClientAppBase::CreateSwapChainRsl,
       SimpleClientAppBase::CreateError>
SimpleClientAppBase::create_swap_chain(const wgpu::Device& device,
                                       GLFWwindow* window, uint32_t width,
                                       uint32_t height) {
#ifdef _WIN32
  WGPUSurfaceDescriptorFromWindowsHWND sdesc{};
  sdesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
  sdesc.hwnd = glfwGetWin32Window(window);
#else
  WGPUSurfaceDescriptorFromXlibWindow sdesc{};
  sdesc.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
  sdesc.display = glfwGetX11Display();
  sdesc.window = glfwGetX11Window(window);
#endif

  WGPUSurfaceDescriptor desc{};
  desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&sdesc);

  WGPUSurface raw_surface = wgpuInstanceCreateSurface(nullptr, &desc);
  wgpu::Surface surface = wgpu::Surface::Acquire(raw_surface);

  wgpu::SwapChainDescriptor swap_chain_desc{};
  swap_chain_desc.format = ::kPreferredTextureFormat;
  swap_chain_desc.usage = wgpu::TextureUsage::RenderAttachment;
  swap_chain_desc.width = width;
  swap_chain_desc.height = height;
  swap_chain_desc.presentMode = wgpu::PresentMode::Fifo;
  wgpu::SwapChain swap_chain =
      device.CreateSwapChain(surface, &swap_chain_desc);

  if (swap_chain == nullptr) {
    return right(CreateError::WGPUSwapChainCreateFailed);
  }

  return left(
      CreateSwapChainRsl{::kPreferredTextureFormat, swap_chain, surface});
}

// Only used in web, no-op in native
void SimpleClientAppBase::set_window_title(const char* name) { return; }
