#ifndef SANCTIFY_COMMON_SIMPLE_CLIENT_APP_SIMPLE_CLIENT_APP_BASE_H
#define SANCTIFY_COMMON_SIMPLE_CLIENT_APP_SIMPLE_CLIENT_APP_BASE_H

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

#include <igcore/config.h>

#ifdef IG_ENABLE_THREADS
#include <igasync/executor_thread.h>
#endif

#include <GLFW/glfw3.h>
#include <igasync/promise.h>
#include <igasync/task_list.h>
#include <igcore/either.h>
#include <igcore/vector.h>
#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp>
#include <string>

/**
 * Simple client app, used for most single-window applications with no special
 * window behavior. Most/all GUI clients should use this class.
 *
 * Web clients assume the existance of a single canvas element with the ID
 *  #app_canvas in the DOM. Render information will be presented there.
 */

namespace sanctify {

enum class MousePressType { Press, Release };

enum class MouseButtonCode { Primary, Secondary };

class ISimpleClientEventListener {
 public:
  // Swap chain change
  virtual void on_swap_chain_resize(uint32_t width, uint32_t height) = 0;
  virtual void on_swap_chain_format_change(
      wgpu::TextureFormat texture_format) = 0;
  virtual void on_enter_fullscreen() = 0;
  virtual void on_exit_fullscreen() = 0;

  // Mouse events
  virtual void on_mouse_click(float x_pct, float y_pct,
                              MouseButtonCode button_code,
                              MousePressType press_type) = 0;
  virtual void on_mouse_move(float x_pct, float y_pct, float dx_pct,
                             float dy_pct) = 0;

  // Keyboard events
  virtual void on_key_press(int glfw_key_code) = 0;
  virtual void on_key_release(int glfw_key_code) = 0;
};

class SimpleClientAppBase {
 public:
  enum class CreateError {
    GLFWInitializationError,
    WindowCreationFailed,
    WGPUDeviceCreationFailed,
    WGPUNoSuitableAdapters,

    WGPUSwapChainCreateFailed,
  };

  SimpleClientAppBase(GLFWwindow* window, wgpu::Device device,
                      wgpu::Surface surface, wgpu::SwapChain swap_chain,
                      uint32_t width, uint32_t height,
                      wgpu::TextureFormat swap_chain_format);
  ~SimpleClientAppBase();

  static indigo::core::Either<std::shared_ptr<SimpleClientAppBase>, CreateError>
  Create(const char* name);

  // Swap chain updates
  wgpu::TextureFormat swap_chain_format() const;
  void resize_swap_chain(uint32_t width, uint32_t height);
  std::shared_ptr<indigo::core::Promise<bool>> enter_fullscreen();
  std::shared_ptr<indigo::core::Promise<bool>> exit_fullscreen();

  // Listeners
  void attach_listener(std::shared_ptr<ISimpleClientEventListener> listener);
  void detach_listener(std::shared_ptr<ISimpleClientEventListener> listener);

  // Async task list attachment - no-ops in the case of a single-threaded
  // environment
  bool supports_threads() const;
  void attach_async_task_list(
      std::shared_ptr<indigo::core::TaskList> task_list);
  void detach_async_task_list(
      std::shared_ptr<indigo::core::TaskList> task_list);

  // Keyboard
  bool is_key_down(int glfw_key_code);

  // Mouse
  bool is_mouse_down(MouseButtonCode button);

 public:
  GLFWwindow* window;
  wgpu::Device device;
  wgpu::Surface surface;
  wgpu::SwapChain swapChain;
  uint32_t width;
  uint32_t height;
  wgpu::TextureFormat swapChainFormat;

 private:
  indigo::core::Vector<std::shared_ptr<ISimpleClientEventListener>> listeners_;

#ifdef IG_ENABLE_THREADS
  indigo::core::Vector<std::shared_ptr<indigo::core::ExecutorThread>>
      executor_threads_;
#endif

 private:
  struct CreateWindowRsl {
    GLFWwindow* window;
    uint32_t w;
    uint32_t h;
  };
  static indigo::core::Either<CreateWindowRsl, CreateError> create_window(
      const char* name);

  static glm::uvec2 get_suggested_window_size();

  static indigo::core::Either<wgpu::Device, CreateError> create_device();

  struct CreateSwapChainRsl {
    wgpu::TextureFormat format;
    wgpu::SwapChain swapChain;
    wgpu::Surface surface;
  };
  static indigo::core::Either<CreateSwapChainRsl, CreateError>
  create_swap_chain(const wgpu::Device& device, GLFWwindow* window,
                    uint32_t width, uint32_t height);

  // Used in web, put in base class for "Create"
  static void set_window_title(const char* name);

  // Events
  void fire_swap_chain_resized_events(uint32_t w, uint32_t h);
};

std::string to_string(SimpleClientAppBase::CreateError err);

}  // namespace sanctify

#endif
