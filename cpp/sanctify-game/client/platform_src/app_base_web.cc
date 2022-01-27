#include <app_base.h>
#include <igcore/log.h>

using indigo::core::Logger;

using namespace indigo;

namespace {
bool gIsGlfwInit = false;

void print_glfw_error(int code, const char* msg) {
  Logger::err("GLFW") << code << " - " << msg;
}
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

Either<std::shared_ptr<AppBase>, AppBaseCreateError> AppBase::Create(
    uint32_t width, uint32_t height) {
  if (!gIsGlfwInit) {
    glfwSetErrorCallback(::print_glfw_error);

    if (!glfwInit()) {
      return core::right(AppBaseCreateError::GLFWInitializationError);
    }
    gIsGlfwInit = true;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window = glfwCreateWindow(width, height, "App Base", nullptr, nullptr);

  WGPUSurfaceDescriptorFromCanvasHTMLSelector canv_desc{};
  canv_desc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
  canv_desc.selector = "#app_canvas";

  WGPUSurfaceDescriptor surface_desc{};
  surface_desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&canv_desc);

  wgpu::Surface surface =
      wgpu::Surface::Acquire(wgpuInstanceCreateSurface(nullptr, &surface_desc));

  wgpu::SwapChainDescriptor swap_desc{};
  swap_desc.usage = wgpu::TextureUsage::RenderAttachment;
  swap_desc.format = wgpu::TextureFormat::BGRA8Unorm;
  swap_desc.width = width;
  swap_desc.height = height;
  swap_desc.presentMode = wgpu::PresentMode::Fifo;

  WGPUDevice raw_device = emscripten_webgpu_get_device();
  if (!raw_device) {
    return core::right(AppBaseCreateError::WGPUDeviceCreationFailed);
  }

  wgpu::Device device = wgpu::Device::Acquire(raw_device);

  wgpu::Queue queue = device.GetQueue();

  wgpu::SwapChain swap_chain = device.CreateSwapChain(surface, &swap_desc);

  auto app_base = std::make_shared<AppBase>(window, device, surface, queue,
                                            swap_chain, width, height);

  return core::left(std::move(app_base));
}

wgpu::TextureFormat AppBase::preferred_swap_chain_texture_format() const {
  return wgpu::TextureFormat::BGRA8Unorm;
}

void AppBase::resize_swap_chain(uint32_t width, uint32_t height) {
  // SwapChain.Configure(preferred_swap_chain_texture_format(),
  //                    wgpu::TextureUsage::RenderAttachment, width, height);
  wgpu::SwapChainDescriptor swap_desc{};
  swap_desc.usage = wgpu::TextureUsage::RenderAttachment;
  swap_desc.format = wgpu::TextureFormat::BGRA8Unorm;
  swap_desc.width = width;
  swap_desc.height = height;
  swap_desc.presentMode = wgpu::PresentMode::Fifo;

  SwapChain = Device.CreateSwapChain(Surface, &swap_desc);

  Width = width;
  Height = height;

  if (SwapChain == nullptr) {
    Logger::err("SwapChainResize") << "Failed to recreate swap chain";
  }
}