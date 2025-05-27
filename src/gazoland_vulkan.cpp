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
}

void gazoland_init() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  //glfwSwapInterval(1);

  {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    std::cout << "available extensions:" << std::endl;
    for (const auto& extension : extensions) {
      std::cout << '\t' << extension.extensionName << '\n';
    }

    std::cout << std::endl << "available validation layers:" << std::endl;
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const auto& layerProperties : availableLayers) {
      std::cout << "\t" << layerProperties.layerName << std::endl;
    }
  }

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
    return;
  }

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    std::cerr << "failed to find GPUs with Vulkan support!" << std::endl;
    return;
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  for (const auto& device : devices) {
    physicalDevice = device;
    break;
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

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
    return;
  }

  vkGetDeviceQueue(device, graphicsFamily.value(), 0, &graphicsQueue);
}

void gazoland_cleanup() {
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
    const std::filesystem::path& path) {
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

void draw_level(
    const std::shared_ptr<program>& terrain_shader,
    const std::vector<float>& projection_matrix,
    float x, float y,
    std::shared_ptr<texture>& stone_tile_texture) {}

void draw_platform(
    const std::shared_ptr<program>& surface_shader,
    const std::shared_ptr<program>& fill_shader,
    const std::shared_ptr<buffer>& vertex_pos_buffer,
    const std::shared_ptr<buffer>& vertex_uv_buffer,
    const std::shared_ptr<buffer>& upper_surface_index_buffer,
    const std::shared_ptr<buffer>& lower_surface_index_buffer,
    const std::shared_ptr<buffer>& corner_vertex_buffer,
    const std::shared_ptr<buffer>& inner_face_index_buffer,
    const std::vector<float>& projection,
    float x, float y, int side_count) {}

void draw_gazo(
    const std::shared_ptr<program>& gazo_shader,
    float x, float y,
    const std::vector<float>& projection_matrix,
    const std::shared_ptr<texture>& gazo_spritesheet_tex,
    const std::shared_ptr<buffer> vertex_buf,
    const std::shared_ptr<buffer> uv_buf,
    const std::shared_ptr<buffer> element_index_buf,
    int uv_map_offset,
    int n_sides) {}

void present_game(
    size_t width, size_t height,
    const std::shared_ptr<program>& gamma_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& draw_tex) {}

void present_gui(
    const std::shared_ptr<program>& gui_prog,
    const std::shared_ptr<buffer>& square_buf,
    const std::shared_ptr<texture>& gui_tex) {}
