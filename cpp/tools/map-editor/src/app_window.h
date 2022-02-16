#ifndef TOOLS_MAP_EDITOR_SRC_APP_WINDOW_H
#define TOOLS_MAP_EDITOR_SRC_APP_WINDOW_H

#include <GLFW/glfw3.h>
#include <igcore/either.h>
#include <webgpu/webgpu_cpp.h>

namespace mapeditor {

enum class MapEditorAppWindowCreateError {
  GLFWInitializationError,
  WindowCreationFailed,
  WGPUDeviceCreationFailed,
  WGPUNoSuitableAdapters,
  WGPUSwapChainCreateFailed,
};

std::string to_string(MapEditorAppWindowCreateError e);

class MapEditorAppWindow {
 public:
  MapEditorAppWindow(GLFWwindow* window, wgpu::Device device,
                     const wgpu::Surface& surface,
                     const wgpu::SwapChain& swap_chain, uint32_t width,
                     uint32_t height)
      : window_(window),
        device_(device),
        surface_(surface),
        swap_chain_(swap_chain),
        width_(width),
        height_(height) {}

  ~MapEditorAppWindow();

  static indigo::core::Either<std::shared_ptr<MapEditorAppWindow>,
                              MapEditorAppWindowCreateError>
  Create(uint32_t width = 0u, uint32_t height = 0u);

  wgpu::TextureFormat swap_chain_texture_format() const;

  void resize_swap_chain(uint32_t width, uint32_t height);

  GLFWwindow* window() const { return window_; }
  const wgpu::Device& device() const { return device_; }
  const wgpu::Surface& surface() const { return surface_; }
  const wgpu::SwapChain& swap_chain() const { return swap_chain_; }
  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }

 private:
  GLFWwindow* window_;
  wgpu::Device device_;
  wgpu::Surface surface_;
  wgpu::SwapChain swap_chain_;
  uint32_t width_;
  uint32_t height_;
};

}  // namespace mapeditor

#endif
