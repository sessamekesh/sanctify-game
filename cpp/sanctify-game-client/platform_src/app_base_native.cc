#include <app_base.h>
#include <dawn/dawn_proc.h>
#include <dawn_native/DawnNative.h>
#include <igcore/log.h>

#include <cassert>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

using namespace indigo;

using indigo::core::Logger;

namespace {
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
  }
  Logger::err("WGPU Device") << "Device lost (" << lost_reason << "): " << msg
                             << "\n   - Developer note: put a breakpoint here";
}

void print_glfw_error(int code, const char* message) {
  Logger::err("GLFW") << code << " - " << message;
}

dawn_native::Adapter get_adapter(
    const std::vector<dawn_native::Adapter>& adapters) {
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

  // Pass 1: Get a discrete GPU if available
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
          return adapter;
        }
      }
    }
  }

  // No suitable adapter found!
  return {};
}

bool gIsGlfwInit = false;

}  // namespace

AppBase::~AppBase() {
  if (Window != nullptr) {
    glfwDestroyWindow(Window);
    Window = nullptr;
  }

  if (gIsGlfwInit) {
    glfwTerminate();
    gIsGlfwInit = false;
  }
}

core::Either<std::shared_ptr<AppBase>, AppBaseCreateError> AppBase::Create(
    uint32_t width, uint32_t height) {
  if (!gIsGlfwInit) {
    glfwSetErrorCallback(::print_glfw_error);
    if (!glfwInit()) {
      return core::right(AppBaseCreateError::GLFWInitializationError);
    }
    gIsGlfwInit = true;
  }

  // Default size if 0u is passed in - find a pleasant resolution
  if (width == 0u || height == 0u) {
    int i_width = 0, i_height = 0;
    auto primary_monitor = glfwGetPrimaryMonitor();
    glfwGetMonitorWorkarea(primary_monitor, nullptr, nullptr, &i_width,
                           &i_height);

    if (i_width >= 1980 && i_height >= 1080) {
      width = 1980u;
      height = 1080u;
    } else if (i_width >= 800 && i_height >= 600) {
      width = 800u;
      height = 600u;
    } else {
      width = 320u;
      height = 200u;
    }
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window = glfwCreateWindow(width, height, "App Base", nullptr, nullptr);

  if (!window) {
    return core::right(AppBaseCreateError::WindowCreationFailed);
  }

  auto instance = std::make_unique<dawn_native::Instance>();
  instance->DiscoverDefaultAdapters();

  dawn_native::Adapter adapter = ::get_adapter(instance->GetAdapters());
  if (!adapter) {
    return core::right(AppBaseCreateError::WGPUNoSuitableAdapters);
  }

  // Feature toggles
  dawn_native::DawnDeviceDescriptor device_descriptor;
  std::vector<const char*> enabled_toggles;

  // Prevents accidental use of SPIR-V, since that isn't supported in
  //  web targets, allegedly because Apple is a piece of shit company that
  //  doesn't give a flying fuck about graphics developers.
  enabled_toggles.push_back("disallow_spirv");

#ifdef IG_ENABLE_GRAPHICS_DEBUGGING
  enabled_toggles.push_back("emit_hlsl_debug_symbols");
  enabled_toggles.push_back("disable_symbol_renaming");
#endif

  device_descriptor.forceEnabledToggles = enabled_toggles;

  WGPUDevice raw_device = adapter.CreateDevice(&device_descriptor);
  if (!raw_device) {
    return core::right(AppBaseCreateError::WGPUDeviceCreationFailed);
  }

  wgpu::Device device = wgpu::Device::Acquire(raw_device);

  DawnProcTable procs = dawn_native::GetProcs();
  dawnProcSetProcs(&procs);

  procs.deviceSetUncapturedErrorCallback(raw_device, ::print_wgpu_device_error,
                                         nullptr);
  procs.deviceSetDeviceLostCallback(raw_device, ::device_lost_callback,
                                    nullptr);

  wgpu::Queue queue = device.GetQueue();

#ifdef _WIN32
  WGPUSurfaceDescriptorFromWindowsHWND sdesc{};
  sdesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
  sdesc.hwnd = glfwGetWin32Window(window);
#else
  WGPUSurfaceDescriptorFromXlib sdesc{};
  sdesc.chain.sType = WGPUSType_SurfaceDescriptorFromXlib;
  sdesc.display = glfwGetX11Display();
  sdesc.window = glfwGetX11Window(window);
#endif

  WGPUSurfaceDescriptor desc{};
  desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&sdesc);

  WGPUSurface raw_surface = wgpuInstanceCreateSurface(nullptr, &desc);
  wgpu::Surface surface = wgpu::Surface::Acquire(raw_surface);

  wgpu::SwapChainDescriptor swap_chain_desc{};
  swap_chain_desc.format = wgpu::TextureFormat::BGRA8Unorm;
  swap_chain_desc.usage = wgpu::TextureUsage::RenderAttachment;
  swap_chain_desc.width = width;
  swap_chain_desc.height = height;
  swap_chain_desc.presentMode = wgpu::PresentMode::Fifo;
  wgpu::SwapChain swap_chain =
      device.CreateSwapChain(surface, &swap_chain_desc);

  if (swap_chain == nullptr) {
    return core::right(AppBaseCreateError::WGPUSwapChainCreateFailed);
  }

  auto app_base = std::make_shared<AppBase>(window, device, surface, queue,
                                            swap_chain, width, height);

  return core::left(std::move(app_base));
}

wgpu::TextureFormat AppBase::preferred_swap_chain_texture_format() const {
  return wgpu::TextureFormat::BGRA8Unorm;
}

void AppBase::resize_swap_chain(uint32_t width, uint32_t height) {
#ifdef _WIN32
  WGPUSurfaceDescriptorFromWindowsHWND sdesc{};
  sdesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
  sdesc.hwnd = glfwGetWin32Window(Window);
#else
  WGPUSurfaceDescriptorFromXlib sdesc{};
  sdesc.chain.sType = WGPUSType_SurfaceDescriptorFromXlib;
  sdesc.display = glfwGetX11Display();
  sdesc.window = glfwGetX11Window(Window);
#endif

  WGPUSurfaceDescriptor desc{};
  desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&sdesc);

  wgpu::SwapChainDescriptor swap_chain_desc{};
  swap_chain_desc.format = wgpu::TextureFormat::BGRA8Unorm;
  swap_chain_desc.usage = wgpu::TextureUsage::RenderAttachment;
  swap_chain_desc.width = width;
  swap_chain_desc.height = height;
  swap_chain_desc.presentMode = wgpu::PresentMode::Fifo;  // TODO (sessamekesh)
  SwapChain = Device.CreateSwapChain(Surface, &swap_chain_desc);
  Width = width;
  Height = height;

  if (SwapChain == nullptr) {
    Logger::err("SwapChainResize") << "Failed to recreate swap chain";
  }
}