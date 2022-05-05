#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <igcore/log.h>

#include "evil_emscripten_hacks.h"
#include "simple_client_app_base.h"

using namespace sanctify;

using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "SimpleClientAppBase.Emscripten";
}

wgpu::TextureFormat SimpleClientAppBase::swap_chain_format() const {
  // This is an evil hack that is only necessary because of how initialization
  //  is done. In a more happy more better world, this could be handled better
  //  by providing an asynchronous initialization of the AppBase that has things
  //  like the adapter reference just ready to go.

  return evil_hacks::get_preferred_texture_format();
}

void SimpleClientAppBase::resize_swap_chain(uint32_t width, uint32_t height) {
  // SwapChain.Configure(preferred_swap_chain_texture_format(),
  //                    wgpu::TextureUsage::RenderAttachment, width, height);
  wgpu::SwapChainDescriptor swap_desc{};
  swap_desc.usage = wgpu::TextureUsage::RenderAttachment;
  swap_desc.format = swap_chain_format();
  swap_desc.width = width;
  swap_desc.height = height;
  swap_desc.presentMode = wgpu::PresentMode::Fifo;

  // TODO (sessamekesh): I don't know if Emscripten or WGPU needs updating for the
  //  GPUCanvasCompositingAlphaMode, but one of them does and that goes here.
  swapChain = device.CreateSwapChain(surface, &swap_desc);

  width = width;
  height = height;

  if (swapChain == nullptr) {
    Logger::err("SwapChainResize") << "Failed to recreate swap chain";
  }

  fire_swap_chain_resized_events(width, height);
}

glm::uvec2 SimpleClientAppBase::get_suggested_window_size() {
  int w, h;
  emscripten_get_canvas_element_size("#app_canvas", &w, &h);

  return glm::uvec2((uint32_t)w, (uint32_t)h);
}

Either<wgpu::Device, SimpleClientAppBase::CreateError>
SimpleClientAppBase::create_device() {
  WGPUDevice raw_device = emscripten_webgpu_get_device();
  if (!raw_device) {
    return right(CreateError::WGPUDeviceCreationFailed);
  }

  return left(wgpu::Device::Acquire(raw_device));
}

Either<SimpleClientAppBase::CreateSwapChainRsl,
       SimpleClientAppBase::CreateError>
SimpleClientAppBase::create_swap_chain(const wgpu::Device& device,
                                       GLFWwindow* window, uint32_t width,
                                       uint32_t height) {
  WGPUSurfaceDescriptorFromCanvasHTMLSelector canv_desc{};
  canv_desc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
  canv_desc.selector = "#app_canvas";

  WGPUSurfaceDescriptor surface_desc{};
  surface_desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&canv_desc);

  wgpu::Surface surface =
      wgpu::Surface::Acquire(wgpuInstanceCreateSurface(nullptr, &surface_desc));

  wgpu::SwapChainDescriptor swap_desc{};
  swap_desc.usage = wgpu::TextureUsage::RenderAttachment;
  swap_desc.format = evil_hacks::get_preferred_texture_format();
  swap_desc.width = width;
  swap_desc.height = height;
  swap_desc.presentMode = wgpu::PresentMode::Fifo;

  wgpu::SwapChain swap_chain = device.CreateSwapChain(surface, &swap_desc);

  return left(CreateSwapChainRsl{swap_desc.format, swap_chain, surface});
}

void SimpleClientAppBase::set_window_title(const char* name) {
  emscripten_set_window_title(name);
}
