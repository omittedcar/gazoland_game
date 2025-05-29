#include "gles_or_vulkan.h"

#include <iostream>
#include <optional>

namespace {
VkInstance instance;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
std::optional<uint32_t> graphicsFamily;
std::optional<uint32_t> computeFamily = 0;
VkDevice device;
VkQueue graphicsQueue;
VkSurfaceKHR surface;

void dumpExtensions() {
  std::cout << "vkGetInstanceProcAddr(instance, \"vkCreateXcbSurfaceKHR\") => "
            << vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR") << std::endl;

  if (!glfwVulkanSupported()) {
    std::cerr << "No vulkan support in GLFW!" << std::endl;
    return;
  }
  {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    std::cout << "available extensions:\n";
    for (const auto& extension : extensions) {
      std::cout << '\t' << extension.extensionName << '\n';
    }
  }

  {
    uint32_t count;
    const char **extensions = glfwGetRequiredInstanceExtensions(&count);
    std::cout << "GLFW requires " << count << " extensions:" << std::endl;
    while (count) {
      std::cout << "\t" << extensions[--count] << std::endl;
    }
  }
}

bool createInstance() {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  VkInstanceCreateInfo instanceCreateInfo{};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = &appInfo;
  instanceCreateInfo.enabledExtensionCount = glfwExtensionCount;
  instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions;
  instanceCreateInfo.enabledLayerCount = 0;

  VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateInstance failed" << std::endl;
    return false;
  }

  return true;
}

bool createSurface() {
  // VkWaylandSurfaceCreateInfoKHR createInfo{};
  // createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
  // createInfo.hwnd = glfwGetWin32Window(window);
  // createInfo.hinstance = GetModuleHandle(nullptr);
  return true;
}

bool pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    std::cerr << "failed to find GPUs with Vulkan support!" << std::endl;
    return false;
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  for (const auto& device : devices) {
    physicalDevice = device;
    break;
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    std::cerr << "failed to find a suitable GPU!" << std::endl;
    return false;
  }

  return true;
}

bool createLogicalDevice() {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
  uint32_t i = 0;
  for (const auto& family : queueFamilies) {
    if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphicsFamily.emplace(i);
    }
    if (family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      computeFamily.emplace(i);
    }
    if (graphicsFamily.has_value() && computeFamily.has_value()) {
      break;
    }
    i++;
  }
  
  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = graphicsFamily.value();
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.f;
  queueCreateInfo.pQueuePriorities = &queuePriority;
  
  VkPhysicalDeviceFeatures deviceFeatures{};
  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
  deviceCreateInfo.enabledExtensionCount = 0;

  if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
    std::cerr << "failed to create logical device!" << std::endl;
    return false;
  }

  vkGetDeviceQueue(device, graphicsFamily.value(), 0, &graphicsQueue);

  return true;
}

}  // namespace {


GLFWwindow* gazoland_init(int width, int height, const char *title) {
  createInstance();

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  //glfwSwapInterval(1);
  GLFWwindow* window =
      glfwCreateWindow(width, height, title, nullptr, nullptr);

  dumpExtensions();

  VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
    std::cerr << "VK_ERROR_EXTENSION_NOT_PRESENT" << std::endl;
  }
  if (result == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR) {
    std::cerr << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << std::endl;
  }
  if (result == VK_ERROR_INITIALIZATION_FAILED) {
    std::cerr << "VK_ERROR_INITIALIZATION_FAILED" << std::endl;
  }

  if (result != VK_SUCCESS) {
    const char* err;
    int code = glfwGetError(&err);
    std::cerr << "Could not create window surface: " << result
              << std::endl << "GLFW: " << err << std::endl;
    return nullptr;
  }

  pickPhysicalDevice();
  createLogicalDevice();

  return window;
}

void gazoland_cleanup(GLFWwindow* window) {
  glfwDestroyWindow(window);
  glfwTerminate();
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
  vkDestroyDevice(device, nullptr);
}

vk_resource::~vk_resource() {}

// static
std::shared_ptr<framebuffer> framebuffer::create(const std::string& name) {
  return std::shared_ptr<framebuffer>(new framebuffer(name));
}

framebuffer::framebuffer(const std::string& name_arg)
    : vk_resource(name_arg) {}

framebuffer::~framebuffer() {}

buffer::buffer(const std::string& name_arg)
    : vk_resource(name_arg) {}

buffer::~buffer() {}
  
// static
std::shared_ptr<shader> shader::create(
    const std::filesystem::path& path,
    shader_type shader_type_arg,
    const std::string& v_pos_name,
    const std::string& v_uv_name) {
  return std::shared_ptr<shader>(new shader(path, v_pos_name, v_uv_name));
}

shader::shader(
    const std::string& name_arg,
    const std::string& v_pos_name,
    const std::string& v_uv_name)
    : vk_resource(name_arg),
      v_pos_name_(v_pos_name),
      v_uv_name_(v_uv_name) {}

shader::~shader() {}

// static
std::shared_ptr<program> program::create(
    const std::string& name,
    std::shared_ptr<shader> vertex_shader,
    std::shared_ptr<shader> fragment_shader,
    const std::string& u_panning_name,
    const std::string& u_projection_name,
    const std::string& u_texture_name) {
  return std::shared_ptr<program>(
      new program(name, vertex_shader, fragment_shader));
}

program::program(
    const std::string& name,
    std::shared_ptr<shader> vertex_shader,
    std::shared_ptr<shader> fragment_shader)
    : vk_resource(name),
      vertex_shader_(vertex_shader),
      fragment_shader_(fragment_shader) {}

program::~program() {}

// static
std::shared_ptr<texture> texture::create_from_file(
    const std::filesystem::path& path,
    size_t mip_count) {
  return std::shared_ptr<texture>(new texture(path));
}

// static
std::shared_ptr<texture> texture::create_for_draw(
    size_t width, size_t height,
    std::shared_ptr<framebuffer> draw_framebuffer) {
  return std::shared_ptr<texture>(new texture("draw_buf"));
}

// static
std::shared_ptr<texture> texture::create_for_depth(
    size_t width, size_t height,
    std::shared_ptr<framebuffer> fb) {
  return std::shared_ptr<texture>(new texture("depth_buf"));
}
  
// static
std::shared_ptr<texture> texture::create_for_gui(
    size_t width, size_t height,
    const std::vector<unsigned char>& lettering) {
  return std::shared_ptr<texture>(new texture("gui"));
}

texture::texture(const std::string& name)
    : vk_resource(name) {}

texture::~texture() {}

void prepare_to_draw(
    const std::shared_ptr<framebuffer>& fb,
    size_t width, size_t height) {}

void draw_platform(
    const platform& pl,
    const std::vector<float>& projection,
    fvec2 view) {}

void draw_gazo(
    const gazo& gz,
    const std::vector<float>& projection_matrix,
    fvec2 view) {}

void present_game(
    size_t width, size_t height,
    const std::shared_ptr<program>& gamma_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& draw_tex) {}

void present_gui(
    const std::shared_ptr<program>& gui_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& gui_tex) {}
