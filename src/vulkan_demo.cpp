#ifdef _WIN32
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#ifdef __linux__
#define VK_USE_PLATFORM_WAYLAND_KHR
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <cstdint>
#include <sysinfoapi.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numbers>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "path.h"

namespace {

void dumpPhysicalDevice(const VkPhysicalDeviceProperties &deviceProperties,
                        const VkPhysicalDeviceFeatures &deviceFeatures) {
  std::cout << "Device:" << std::endl
            << "\t" << "apiVersion=" << deviceProperties.apiVersion << std::endl
            << "\t" << "driverVersion=" << deviceProperties.driverVersion
            << std::endl
            << "\t" << "vendorID=" << deviceProperties.vendorID << std::endl
            << "\t" << "deviceID=" << deviceProperties.deviceID << std::endl
            << "\t" << "deviceType=" << deviceProperties.deviceType << std::endl
            << "\t" << "geometryShader=" << deviceFeatures.geometryShader
            << std::endl
            << "\t" << "textureCompressionETC2="
            << deviceFeatures.textureCompressionETC2 << std::endl
            << "\t" << "shaderFloat64=" << deviceFeatures.shaderFloat64
            << std::endl
            << "\t" << "shaderInt64=" << deviceFeatures.shaderInt64
            << std::endl;
}

int PhysicalDeviceScore(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
  dumpPhysicalDevice(deviceProperties, deviceFeatures);
  int result = 0;
  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    result += 1000;
  }
  return result;
};

std::pair<std::optional<uint32_t>, std::optional<uint32_t>>
PhysicalDeviceQueues(VkPhysicalDevice device, VkSurfaceKHR surface) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());
  std::optional<uint32_t> graphics_queue_num;
  std::optional<uint32_t> presentation_queue_num;
  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphics_queue_num = i;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport) {
      presentation_queue_num = i;
    }
  }
  return {graphics_queue_num, presentation_queue_num};
};

bool CheckPhysicalDeviceExtensions(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());
  for (const auto &extension : availableExtensions) {
    if (!strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
      return true;
    }
  }
  return false;
};

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(GLFWwindow *window,
                            const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats.data());
  }
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.presentModes.data());
  }
  return details;
}

VkShaderModule loadShaderFromFile(VkDevice device,
  const char *shader_name) {
  std::filesystem::path shader_path(root_path());
  shader_path /= "assets";
  shader_path /= "spirv";
  shader_path /= std::string(shader_name) + ".spv";
  std::ifstream ifs(shader_path, std::ios::ate | std::ios::binary);
  if (ifs.fail()) {
    int errnum = errno;
    DWORD win32Err = ::GetLastError();
    std::cerr << "Error: Failed to open file: " << shader_path << "\n";
    char errBuffer[256];
    if (strerror_s(errBuffer, sizeof(errBuffer), errnum) == 0) {
      std::cerr << "CRT Reason: " << errBuffer << " (errno: " << errnum
                << ")\n";
    }
    if (win32Err != ERROR_SUCCESS) {
      LPSTR messageBuffer = nullptr;
      size_t size = FormatMessageA(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
              FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL, win32Err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPSTR)&messageBuffer, 0, NULL);
      if (size > 0) {
        std::cerr << "Win32 Reason: " << messageBuffer << " (Code: " << win32Err
                  << ")\n";
        LocalFree(messageBuffer); // Free allocated buffer
      }
    }
  }
  std::vector<char> buf(ifs.tellg());
  ifs.seekg(0, std::ios::beg);
  ifs.read(buf.data(), static_cast<std::streamsize>(buf.size()));
  ifs.close();
  VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = buf.size(),
      .pCode = reinterpret_cast<const uint32_t *>(buf.data())};
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
  return shaderModule;
}

} // namespace

class HelloTriangleApplication {
public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  static void framebufferResizeCallback(
      GLFWwindow* window, int width, int height) {
    auto* app = reinterpret_cast<HelloTriangleApplication*>(
        glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
  }

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  }

  void initVulkan() {
    dumpExtensions();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createCommandBuffers();
    createSyncObjects();
  }

  void mainLoop() {
    startTicks = GetTickCount64();
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      drawFrame();
    }
    vkDeviceWaitIdle(device);
  }

  void cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
      vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
      vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    cleanupSwapChain();
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    std::vector<const char *> enabledLayers;
    createInfo.enabledLayerCount = 0;
    if (checkValidationLayerSupport("VK_LAYER_KHRONOS_validation")) {
      createInfo.enabledLayerCount++;
      enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
    if (createInfo.enabledLayerCount) {
      createInfo.ppEnabledLayerNames = enabledLayers.data();
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
      throw std::runtime_error("failed to create instance!");
    }
  }

  void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create surface!");
    }
  }

  void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    std::vector<std::pair<int, VkPhysicalDevice>> rankedDevices;
    for (const auto &device : devices) {
      rankedDevices.emplace_back(PhysicalDeviceScore(device), device);
    }
    std::sort(rankedDevices.begin(), rankedDevices.end());
    SwapChainSupportDetails details;
    for (const auto &value : rankedDevices | std::views::reverse) {
      auto queues = PhysicalDeviceQueues(value.second, surface);
      bool extensionsOk = CheckPhysicalDeviceExtensions(value.second);
      bool swapChainOk = false;
      if (extensionsOk) {
        details = querySwapChainSupport(value.second, surface);
        swapChainOk = !details.formats.empty() & !details.presentModes.empty();
      }
      if (queues.first.has_value() && queues.second.has_value() &&
          extensionsOk && swapChainOk) {
        physicalDevice = value.second;
        graphicsQueueNum = queues.first.value();
        presentationQueueNum = queues.second.value();
        swapChainDetails = details;
        break;
      }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
      throw std::runtime_error("failed to find a suitable GPU!");
    }
  }

  void createLogicalDevice() {
    float priority = 1.0;
    VkDeviceQueueCreateInfo queueCreateInfos[2]{
        {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
         .pNext = nullptr,
         .flags = 0,
         .queueFamilyIndex = graphicsQueueNum,
         .queueCount = 1,
         .pQueuePriorities = &priority},
        {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
         .pNext = nullptr,
         .flags = 0,
         .queueFamilyIndex = presentationQueueNum,
         .queueCount = 1,
         .pQueuePriorities = &priority}};
    VkPhysicalDeviceFeatures deviceFeatures{};
    const char *requiredExtensions[]{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                  .queueCreateInfoCount = 2,
                                  .pQueueCreateInfos = queueCreateInfos,
                                  .enabledLayerCount = 0,
                                  .enabledExtensionCount = 1,
                                  .ppEnabledExtensionNames = requiredExtensions,
                                  .pEnabledFeatures = &deviceFeatures};
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(device, graphicsQueueNum, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentationQueueNum, 0, &presentationQueue);
  }

  void createSwapChain() {
    VkSurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(swapChainDetails.formats);
    swapChainImageFormat = surfaceFormat.format;
    VkPresentModeKHR presentMode =
        chooseSwapPresentMode(swapChainDetails.presentModes);
    swapChainExtent = chooseSwapExtent(window, swapChainDetails.capabilities);
    uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;
    if (swapChainDetails.capabilities.maxImageCount) {
      imageCount =
          std::min(imageCount, swapChainDetails.capabilities.maxImageCount);
    }
    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = swapChainImageFormat,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapChainDetails.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE};
    uint32_t queueFamilyIndices[] = {graphicsQueueNum, presentationQueueNum};
    if (graphicsQueueNum != presentationQueueNum) {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;
      createInfo.pQueueFamilyIndices = nullptr;
    }
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create swap chain!");
    }
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                            swapChainImages.data());
  }

  void cleanupSwapChain() {
    for (auto framebuffer : swapChainFramebuffers) {
      vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (auto imageView : swapChainImageViews) {
      vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
  }

  void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
      glfwWaitEvents();
      glfwGetFramebufferSize(window, &width, &height);
    }
    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createFramebuffers();
  }

  void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
      VkImageViewCreateInfo createInfo{
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = swapChainImages[i],
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = swapChainImageFormat};
      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;
      if (vkCreateImageView(device, &createInfo, nullptr,
                            &swapChainImageViews[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image views!");
      }
    }
  }

  void createRenderPass() {
    VkAttachmentDescription colorAttachment{
        .format = swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference colorAttachmentRef{
        .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{.pipelineBindPoint =
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 .colorAttachmentCount = 1,
                                 .pColorAttachments = &colorAttachmentRef};
    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency};

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create render pass!");
    }
  }

  void createGraphicsPipeline() {
    VkShaderModule vertShader = loadShaderFromFile(device, "vulkan_demo_vert");
    VkShaderModule fragShader = loadShaderFromFile(device, "vulkan_demo_frag");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShader,
        .pName = "main"};
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShader,
        .pName = "main"};
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()};

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE};

    VkViewport viewport{.x = 0.0f,
                        .y = 0.0f,
                        .width = (float)swapChainExtent.width,
                        .height = (float)swapChainExtent.height,
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f};

    VkRect2D scissor{.offset = {0, 0}, .extent = swapChainExtent};

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor};

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp = 0.f,
        .depthBiasSlopeFactor = 0.f,
        .lineWidth = 1.0};

    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE};

    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {0.0f, 0.0f, 0.f, 0.f}};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,             // Optional
        .pSetLayouts = nullptr,          // Optional
        .pushConstantRangeCount = 0,     // Optional
        .pPushConstantRanges = nullptr}; // Optional
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr, // Optional
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1};
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, vertShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
  }

  void createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
      VkImageView attachments[] = {swapChainImageViews[i]};

      VkFramebufferCreateInfo framebufferInfo{
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .renderPass = renderPass,
          .attachmentCount = 1,
          .pAttachments = attachments,
          .width = swapChainExtent.width,
          .height = swapChainExtent.height,
          .layers = 1};

      if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                              &swapChainFramebuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
      }
    }
  }

  void createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphicsQueueNum};
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool!");
    }
  }

  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }

    throw std::runtime_error("failed to find suitable memory type!");
  }

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                  .size = size,
                                  .usage = usage,
                                  .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to create buffer!");
    }
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits, properties)};
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to allocate buffer memory!");
    }
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
  }

  void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // void *data;
    // vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    // memcpy(data, vertices.data(), (size_t)bufferSize);
    // vkUnmapMemory(device, stagingBufferMemory);
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    //copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
  }

  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1};
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    VkBufferCopy copyRegion{.srcOffset = 0, // Optional
                            .dstOffset = 0, // Optional
                            .size = size};
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                            .commandBufferCount = 1,
                            .pCommandBuffers = &commandBuffer};
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  }

  void createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 2};

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers!");
    }
  }

  void bufferVertexData() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
  }

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,                   // Optional
        .pInheritanceInfo = nullptr}; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }
    VkClearValue clearColor = {{{0.f, 0.f, 0.f, 0.f}}};
    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = swapChainFramebuffers[imageIndex],
        .renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
        .clearValueCount = 1,
        .pClearValues = &clearColor};

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    VkViewport viewport{.x = 0.0f,
                        .y = 0.0f,
                        .width = static_cast<float>(swapChainExtent.width),
                        .height = static_cast<float>(swapChainExtent.height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{.offset = {0, 0}, .extent = swapChainExtent};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }

  void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                            &imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                            &renderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
              VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphores!");
      }
    }
  }

  void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);
    ULONGLONG nowTicks = GetTickCount64();
    float progress = (((nowTicks - startTicks) % 4000) / 4000.f);
    float angle = progress * 2 * std::numbers::pi;
    float color = (cosf(angle) + 1.f) / 2.f;
    vertices[0].color = {1.0f, color, color};
    vertices[1].color = {color, 1.0f, color};
    vertices[2].color = {color, color, 1.0f};
    bufferVertexData();
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain();
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                            .waitSemaphoreCount = 1,
                            .pWaitSemaphores = waitSemaphores,
                            .pWaitDstStageMask = waitStages,
                            .commandBufferCount = 1,
                            .pCommandBuffers = &commandBuffers[currentFrame],
                            .signalSemaphoreCount = 1,
                            .pSignalSemaphores = signalSemaphores};
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                      inFlightFences[currentFrame]) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
    }
    VkSwapchainKHR swapChains[] = {swapChain};
    VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                 .waitSemaphoreCount = 1,
                                 .pWaitSemaphores = signalSemaphores,
                                 .swapchainCount = 1,
                                 .pSwapchains = swapChains,
                                 .pImageIndices = &imageIndex,
                                 .pResults = nullptr};
    result = vkQueuePresentKHR(presentationQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        framebufferResized) {
      framebufferResized = false;
      recreateSwapChain();
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void dumpExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                           extensions.data());
    std::cout << "available extensions:" << std::endl;
    for (const auto &extension : extensions) {
      std::cout << '\t' << extension.extensionName << std::endl;
    }
    std::cout << "Swapchain Extension = " << VK_KHR_SWAPCHAIN_EXTENSION_NAME
              << std::endl;
  }

  bool checkValidationLayerSupport(const char *layerName) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    bool result = false;
    std::cout << "Validation Layers:" << std::endl;
    for (const auto &layer : availableLayers) {
      std::cout << "\t" << layer.layerName << std::endl;
      if (!strcmp(layer.layerName, layerName)) {
        result = true;
      }
    }
    return result;
  }

  const size_t MAX_FRAMES_IN_FLIGHT = 2;
  ULONGLONG startTicks;
  GLFWwindow *window = nullptr;
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  uint32_t graphicsQueueNum = std::numeric_limits<uint32_t>::max();
  uint32_t presentationQueueNum = std::numeric_limits<uint32_t>::max();
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentationQueue = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  SwapChainSupportDetails swapChainDetails;
  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  size_t currentFrame = 0;
  bool framebufferResized = false;

  struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    static VkVertexInputBindingDescription getBindingDescription() {
      VkVertexInputBindingDescription bindingDescription{
          .binding = 0,
          .stride = sizeof(Vertex),
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
      return bindingDescription;
    }
    static std::array<VkVertexInputAttributeDescription, 2>
    getAttributeDescriptions() {
      std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{{
          {.location = 0,
           .binding = 0,
           .format = VK_FORMAT_R32G32_SFLOAT,
           .offset = offsetof(Vertex, pos)},
          {.location = 1,
           .binding = 0,
           .format = VK_FORMAT_R32G32B32_SFLOAT,
           .offset = offsetof(Vertex, color)}}};
      return attributeDescriptions;
    }
  };
  std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.f, 0.f, 1.f}}
  };
};

int main(int argc, char **argv) {
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
