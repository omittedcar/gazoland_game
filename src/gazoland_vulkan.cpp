#include "gles_or_vulkan.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <string.h>

namespace {

void logGlfwError(const char* context) {
  const char* description;
  int code = glfwGetError(&description);
  std::cerr << context << " failed with code " << code
            << ": " << description << std::endl;
}

struct QueueFamilies {
  uint32_t graphics_ = 0;
  uint32_t present_ = 0;
  uint32_t compute_ = 0;
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities_{};
  std::vector<VkSurfaceFormatKHR> formats_;
  std::vector<VkPresentModeKHR> present_modes_;
};

class VulkanApp {
 public:
  VulkanApp(uint32_t width, uint32_t height, const char* title) {
    valid_ =
        createWindow(width, height, title) &&
        createInstance() &&
        createSurface() &&
        pickPhysicalDevice() &&
        createLogicalDevice() &&
        createSwapChain();
  }
  ~VulkanApp();

  bool valid() const { return valid_; }
  GLFWwindow* window() const { return valid_ ? window_ : nullptr; }
  
 private:
  bool createWindow(uint32_t width, uint32_t height, const char* title);
  bool createInstance();
  bool createSurface();
  bool isDeviceSuitable(
      VkPhysicalDevice pd,
      QueueFamilies& queue_families,
      SwapChainSupportDetails& swap_info);
  bool pickPhysicalDevice();
  bool createLogicalDevice();
  bool createSwapChain();

  bool valid_ = false;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;
  VkQueue compute_queue_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  SwapChainSupportDetails swap_chain_details_;
  VkSwapchainKHR swap_chain_ = VK_NULL_HANDLE;
  std::vector<VkImage> swap_chain_images_;
  std::vector<VkImageView> swap_chain_image_views_;
  VkFormat swap_chain_image_format_;
  VkExtent2D swap_chain_extent_;
  GLFWwindow* window_ = nullptr;
  QueueFamilies queue_families_;
};

bool VulkanApp::createWindow(uint32_t width, uint32_t height, const char* title) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window_) {
    logGlfwError(__FUNCTION__);
    return false;
  }
  return true;
}

VulkanApp::~VulkanApp() {
  if (window_) {
    glfwDestroyWindow(window_);
  }
  glfwTerminate();
  for (uint32_t i = 0; i < swap_chain_image_views_.size(); i++) {
    vkDestroyImageView(device_, swap_chain_image_views_[i], nullptr);
  }
  if (surface_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
  }
  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
  }
  if (device_ != VK_NULL_HANDLE) {
    vkDestroyDevice(device_, nullptr);
  }
  if (swap_chain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
  }
}

bool VulkanApp::createInstance() {
  VkApplicationInfo appInfo{
    VK_STRUCTURE_TYPE_APPLICATION_INFO,  //sType
    nullptr,  //pNext
    "gazoland",  //pApplicationName
    VK_MAKE_VERSION(1, 0, 0),  //applicationVersion
    nullptr,  //pEngineName
    VK_MAKE_VERSION(1, 0, 0),  //engineVersion
    VK_API_VERSION_1_0  //apiVersion
  };

  std::vector<const char*> enabled_layer_names;

#ifndef NDEBUG
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
  for (const auto& layerProperties : available_layers) {
    if (strcmp("VK_LAYER_KHRONOS_validation", layerProperties.layerName) == 0) {
      enabled_layer_names.push_back("VK_LAYER_KHRONOS_validation");
      break;
    }
  }
#endif
  
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  VkInstanceCreateInfo instanceCreateInfo{
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,  //sType
    nullptr,  //pNext
    0,  //flags
    &appInfo,  //pApplicationInfo
    static_cast<uint32_t>(enabled_layer_names.size()),  //enabledLayerCount
    enabled_layer_names.data(),  //ppEnabledLayerNames
    glfwExtensionCount,  //enabledExtensionCount
    glfwExtensions  //ppEnabledExtensionNames
  };

  VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance_);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateInstance failed (" << result << ")" << std::endl;
    return false;
  }
  return true;
}

bool VulkanApp::createSurface() {
  if (glfwCreateWindowSurface(
          instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
    logGlfwError(__FUNCTION__);
    return false;
  }
  return true;
}

bool VulkanApp::isDeviceSuitable(
    VkPhysicalDevice pd,
    QueueFamilies& queue_families,
    SwapChainSupportDetails& swap_info) {
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(pd, &deviceProperties);
  vkGetPhysicalDeviceFeatures(pd, &deviceFeatures);
  // more checks?

  std::optional<uint32_t> pdGraphicsFamily;
  std::optional<uint32_t> pdPresentFamily;
  std::optional<uint32_t> pdComputeFamily;
  bool swapchain_support = false;
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(
      pd, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(
      pd, nullptr, &extensionCount, availableExtensions.data());
  for (const auto& ext : availableExtensions) {
    if (!strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
      swapchain_support = true;
      break;
    }
  }
  if (!swapchain_support) {
    return false;
  }
  
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      pd, surface_, &swap_chain_details_.capabilities_);
  
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface_, &formatCount, nullptr);
  if (formatCount != 0) {
    swap_info.formats_.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        pd, surface_, &formatCount, swap_info.formats_.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      pd, surface_, &presentModeCount, nullptr);
  if (presentModeCount != 0) {
    swap_info.present_modes_.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        pd, surface_, &presentModeCount,
        swap_info.present_modes_.data());
  }

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(pd, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      pd, &queueFamilyCount, queueFamilies.data());
  uint32_t i = 0;
  for (const auto& family : queueFamilies) {
    if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queue_families.graphics_ = i;
      pdGraphicsFamily.emplace(i);
    }
    if (family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      pdComputeFamily.emplace(i);
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        pd, i, surface_, &presentSupport);
    if (presentSupport) {
      pdPresentFamily.emplace(i);
    }
    if (pdGraphicsFamily.has_value() && pdComputeFamily.has_value()
        && pdPresentFamily.has_value()) {
      queue_families.graphics_ = pdGraphicsFamily.value();
      queue_families.present_ = pdPresentFamily.value();
      queue_families.compute_ = pdComputeFamily.value();
      return true;
    }
    i++;
  }
  return false;
}

bool VulkanApp::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
  if (deviceCount == 0) {
    std::cerr << "failed to find GPUs with Vulkan support!" << std::endl;
    return false;
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
  for (const auto& device : devices) {
    if (isDeviceSuitable(device, queue_families_, swap_chain_details_)) {
      physical_device_ = device;
      break;
    }
  }
  
  if (physical_device_ == VK_NULL_HANDLE) {
    std::cerr << "failed to find a suitable GPU!" << std::endl;
    return false;
  }

  return true;
}

bool VulkanApp::createLogicalDevice() {
  float queuePriority = 1.f;
  VkDeviceQueueCreateInfo queueCreateInfos[2]{
    { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      nullptr,
      0,
      queue_families_.graphics_,
      1,
      &queuePriority },
    { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      nullptr,
      0,
      queue_families_.present_,
      1,
      &queuePriority } }; 
  VkDeviceQueueCreateInfo foo{};

  const char* enabled_extension_names[1]{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  VkPhysicalDeviceFeatures deviceFeatures{};
  VkDeviceCreateInfo deviceCreateInfo{
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    nullptr,
    0,
    queue_families_.graphics_ == queue_families_.present_ ? 1u : 2u,
    queueCreateInfos,
    0,
    nullptr,
    1,  // enabledExtensionCount
    enabled_extension_names,  // ppEnabledExtensionNames
    &deviceFeatures};
  
  if (vkCreateDevice(
          physical_device_, &deviceCreateInfo, nullptr, &device_) != VK_SUCCESS) {
    std::cerr << "failed to create logical device!" << std::endl;
    return false;
  }

  vkGetDeviceQueue(device_, queue_families_.graphics_, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, queue_families_.present_, 0, &present_queue_);
  vkGetDeviceQueue(device_, queue_families_.present_, 0, &compute_queue_);

  return true;
}

bool VulkanApp::createSwapChain() {
  int w, h;
  glfwGetFramebufferSize(window_, &w, &h);
  swap_chain_extent_ = {
    std::clamp(static_cast<uint32_t>(w),
               swap_chain_details_.capabilities_.minImageExtent.width,
               swap_chain_details_.capabilities_.maxImageExtent.width),
    std::clamp(static_cast<uint32_t>(h),
               swap_chain_details_.capabilities_.minImageExtent.height,
               swap_chain_details_.capabilities_.maxImageExtent.height)};
  
  VkSurfaceFormatKHR surface_format = swap_chain_details_.formats_[0];
  for (const auto& format : swap_chain_details_.formats_) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      surface_format = format;
      break;
    }
  }
  swap_chain_image_format_ = surface_format.format;

  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for (const auto& mode : swap_chain_details_.present_modes_) {
    if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      present_mode = mode;
      break;
    }
  }

  uint32_t image_count = swap_chain_details_.capabilities_.minImageCount + 1;
  if (swap_chain_details_.capabilities_.maxImageCount > 0 &&
      image_count > swap_chain_details_.capabilities_.maxImageCount) {
    image_count = swap_chain_details_.capabilities_.maxImageCount;
  }
 
  VkSwapchainCreateInfoKHR create_info{
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // sType
    nullptr,  //pNext
    0,  //flags
    surface_,  //surface
    image_count,  //minImageCount
    surface_format.format,  //imageFormat
    surface_format.colorSpace,  //imageColorSpace
    swap_chain_extent_,  //imageExtent
    1,  //imageArrayLayers
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,  //imageUsage
    VK_SHARING_MODE_EXCLUSIVE,  //imageSharingMode
    0,  //queueFamilyIndexCount
    nullptr,  //pQueueFamilyIndices
    swap_chain_details_.capabilities_.currentTransform,  //preTransform
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,  //compositeAlpha
    present_mode,  //presentMode
    VK_TRUE,  //clipped
    VK_NULL_HANDLE  //oldSwapchain
  };
  if (vkCreateSwapchainKHR(
          device_, &create_info, nullptr, &swap_chain_) != VK_SUCCESS) {
    std::cerr << "Could not create swap chain." << std::endl;
    return false;
  }

  vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, nullptr);
  swap_chain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(
      device_, swap_chain_, &image_count, swap_chain_images_.data());
  swap_chain_image_views_.resize(image_count);
  for (uint32_t i = 0; i < image_count; i++) {
    VkImageViewCreateInfo view_create_info{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  //sType
      nullptr,  //pNext
      0,  //flags
      swap_chain_images_[i],  //image
      VK_IMAGE_VIEW_TYPE_2D,  //viewType
      swap_chain_image_format_,  //format
      { VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY },  //components
      {
        VK_IMAGE_ASPECT_COLOR_BIT,  //aspectMask
        0,  //baseMipLevel
        1,  //levelCount
        0,  //baseArrayLayer
        1,  //layerCount
      }};  //subresourceRange
    if (vkCreateImageView(
            device_, &view_create_info, nullptr, &swap_chain_image_views_[i])
        != VK_SUCCESS) {
      swap_chain_image_views_.resize(i);
      std::cerr << "Could not create swap chain image view" << std::endl;
      return false;
    }
  }

  return true;
}

std::unique_ptr<VulkanApp> the_app;

}  // namespace {


GLFWwindow* gazoland_init(int width, int height, const char *title) {
  if (!glfwInit()) {
    int code = glfwGetError(NULL);
    std::cerr << "glfwInitFailed, error code " << code << std::endl;
    return nullptr;
  }

  the_app = std::make_unique<VulkanApp>(width, height, title);
  if (the_app->valid()) {
    return the_app->window();
  }
  return nullptr;  
}

void gazoland_cleanup(GLFWwindow* window) {
  the_app.reset();
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
