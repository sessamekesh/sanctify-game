#ifndef SANCTIFY_GAME_CLIENT_PLATFORM_SRC_APP_BASE_H
#define SANCTIFY_GAME_CLIENT_PLATFORM_SRC_APP_BASE_H

#ifdef __EMSCRIPTEN__
/**
 * Make sure Emscripten is up to date enough to support WebGPU
 */
#if __EMSCRIPTEN_major__ == 1 &&  \
    (__EMSCRIPTEN_minor__ < 40 || \
     (__EMSCRIPTEN_minor__ == 40 && __EMSCRIPTEN_tiny__ < 1))
#error "Emscripten 1.40.1 or higher required"
#endif
#include <emscripten/html5_webgpu.h>
#endif

#include <GLFW/glfw3.h>
#include <igcore/either.h>
#include <webgpu/webgpu_cpp.h>

#include <memory>
#include <string>
#include <unordered_map>

using indigo::core::Either;

enum class AppBaseCreateError {
  GLFWInitializationError,
  WindowCreationFailed,
  WGPUDeviceCreationFailed,
  WGPUNoSuitableAdapters,

  WGPUSwapChainCreateFailed,
};

inline std::string app_base_create_error_text(AppBaseCreateError err) {
  switch (err) {
    case AppBaseCreateError::GLFWInitializationError:
      return "GLFWInitializationError";
    case AppBaseCreateError::WindowCreationFailed:
      return "WindowCreationFailed";
    case AppBaseCreateError::WGPUNoSuitableAdapters:
      return "WGPUNoSuitableAdapters";
    case AppBaseCreateError::WGPUSwapChainCreateFailed:
      return "WGPUSwapChainCreateFailed";
    case AppBaseCreateError::WGPUDeviceCreationFailed:
      return "WGPUDeviceCreationFailed";
    default:
      return "UNKNOWN";
  }
}

struct AppBase {
 public:
  AppBase(GLFWwindow* window, wgpu::Device device, const wgpu::Surface& surface,
          const wgpu::Queue& queue, const wgpu::SwapChain& swap_chain,
          uint32_t width, uint32_t height)
      : Window(window),
        Device(device),
        Surface(surface),
        Queue(queue),
        SwapChain(swap_chain),
        Width(width),
        Height(height) {}

  ~AppBase();

  static Either<std::shared_ptr<AppBase>, AppBaseCreateError> Create(
      uint32_t width = 0u, uint32_t height = 0u);

  wgpu::TextureFormat preferred_swap_chain_texture_format() const;

  void resize_swap_chain(uint32_t width, uint32_t height);

 public:
  GLFWwindow* Window;
  wgpu::Device Device;
  wgpu::Surface Surface;
  wgpu::Queue Queue;
  wgpu::SwapChain SwapChain;
  uint32_t Width;
  uint32_t Height;
};

#endif