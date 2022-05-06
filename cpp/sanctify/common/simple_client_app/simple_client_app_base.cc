#include "simple_client_app_base.h"

using namespace sanctify;

using namespace indigo;
using namespace core;

namespace {
void print_glfw_error(int code, const char* msg) {
  Logger::err("GLFW") << code << " - " << msg;
}
}  // namespace

SimpleClientAppBase::SimpleClientAppBase(GLFWwindow* window,
                                         wgpu::Device device,
                                         wgpu::Surface surface,
                                         wgpu::SwapChain swap_chain,
                                         uint32_t width, uint32_t height,
                                         wgpu::TextureFormat swap_chain_format)
    : window(window),
      device(device),
      surface(surface),
      swapChain(swap_chain),
      width(width),
      height(height),
      swapChainFormat(swap_chain_format),
      listeners_(1)
#ifdef IG_ENABLE_THREADS
      ,
      executor_threads_(std::thread::hardware_concurrency())
#endif
{
#ifdef IG_ENABLE_THREADS
  for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
    executor_threads_.push_back(std::make_shared<ExecutorThread>());
  }
#endif
}

SimpleClientAppBase::~SimpleClientAppBase() {
  if (window) {
    glfwDestroyWindow(window);
    window = nullptr;
  }
  glfwTerminate();
}

Either<std::shared_ptr<SimpleClientAppBase>, SimpleClientAppBase::CreateError>
SimpleClientAppBase::Create(const char* name) {
  auto window_rsl = SimpleClientAppBase::create_window(name);

  if (window_rsl.is_right()) {
    return right(window_rsl.right_move());
  }

  GLFWwindow* window = window_rsl.get_left().window;
  uint32_t width = window_rsl.get_left().w;
  uint32_t height = window_rsl.get_left().h;

  auto device_rsl = SimpleClientAppBase::create_device();
  if (device_rsl.is_right()) {
    return right(device_rsl.right_move());
  }

  wgpu::Device device = device_rsl.left_move();

  auto swap_chain_rsl =
      SimpleClientAppBase::create_swap_chain(device, window, width, height);
  if (swap_chain_rsl.is_right()) {
    return right(device_rsl.right_move());
  }

  wgpu::Surface surface = swap_chain_rsl.get_left().surface;
  wgpu::SwapChain swap_chain = swap_chain_rsl.get_left().swapChain;
  wgpu::TextureFormat swap_chain_format = swap_chain_rsl.get_left().format;

  SimpleClientAppBase::set_window_title(name);

  return left(std::make_shared<SimpleClientAppBase>(
      window, device, surface, swap_chain, width, height, swap_chain_format));
}

Either<SimpleClientAppBase::CreateWindowRsl, SimpleClientAppBase::CreateError>
SimpleClientAppBase::create_window(const char* name) {
  glfwSetErrorCallback(::print_glfw_error);

  // glfwInit may be called multiple times, and will immediately return true if
  //  the library has been successfully initialized and not yet shut down.
  if (!glfwInit()) {
    return right(CreateError::GLFWInitializationError);
  }

  auto window_dimensions = SimpleClientAppBase::get_suggested_window_size();
  uint32_t width = window_dimensions.x;
  uint32_t height = window_dimensions.y;

  // We'll handle creating the graphics API handles, since we're using WebGPU
  //  and GLFW is designed to work with OpenGL and OpenGL ES.
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window = glfwCreateWindow(width, height, name, nullptr, nullptr);

  if (!window) {
    return right(CreateError::WindowCreationFailed);
  }

  return left(CreateWindowRsl{window, width, height});
}

void SimpleClientAppBase::fire_swap_chain_resized_events(uint32_t w,
                                                         uint32_t h) {
  for (int i = 0; i < listeners_.size(); i++) {
    listeners_[i]->on_swap_chain_resize(w, h);
  }
  width = w;
  height = h;
}

void SimpleClientAppBase::attach_listener(
    std::shared_ptr<ISimpleClientEventListener> listener) {
  listeners_.erase(listener, false);
  listeners_.push_back(listener);
}

void SimpleClientAppBase::detach_listener(
    std::shared_ptr<ISimpleClientEventListener> listener) {
  listeners_.erase(listener, false);
}

bool SimpleClientAppBase::supports_threads() const {
#ifdef IG_ENABLE_THREADS
  return true;
#else
  return false;
#endif
}

void SimpleClientAppBase::attach_async_task_list(
    std::shared_ptr<TaskList> task_list) {
#ifdef IG_ENABLE_THREADS
  for (int i = 0; i < executor_threads_.size(); i++) {
    executor_threads_[i]->add_task_list(task_list);
  }
#endif
}

void SimpleClientAppBase::detach_async_task_list(
    std::shared_ptr<TaskList> task_list) {
#ifdef IG_ENABLE_THREADS
  for (int i = 0; i < executor_threads_.size(); i++) {
    executor_threads_[i]->remove_task_list(task_list);
  }
#endif
}

bool SimpleClientAppBase::is_key_down(int glfw_key_code) {
  return glfwGetKey(window, glfw_key_code) == GLFW_PRESS;
}

bool SimpleClientAppBase::is_mouse_down(MouseButtonCode code) {
  int glfw_code = (code == MouseButtonCode::Primary) ? GLFW_MOUSE_BUTTON_LEFT
                                                     : GLFW_MOUSE_BUTTON_RIGHT;

  return glfwGetMouseButton(window, glfw_code) == GLFW_PRESS;
}

std::string sanctify::to_string(SimpleClientAppBase::CreateError err) {
  switch (err) {
    case SimpleClientAppBase::CreateError::GLFWInitializationError:
      return "GLFWInitializationError";
    case SimpleClientAppBase::CreateError::WindowCreationFailed:
      return "WindowCreationFailed";
    case SimpleClientAppBase::CreateError::WGPUNoSuitableAdapters:
      return "WGPUNoSuitableAdapters";
    case SimpleClientAppBase::CreateError::WGPUSwapChainCreateFailed:
      return "WGPUSwapChainCreateFailed";
    case SimpleClientAppBase::CreateError::WGPUDeviceCreationFailed:
      return "WGPUDeviceCreationFailed";
  }

  return "UNKNOWN";
}
