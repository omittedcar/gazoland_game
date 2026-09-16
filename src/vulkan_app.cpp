#ifdef _WIN32
#include <windows.h>
#endif

#include "vulkan_app.h"
#include <cstdint>
#include <string>

#ifdef _WIN32
#include <sysinfoapi.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#ifdef __linux__
#include <time.h>
#define VK_USE_PLATFORM_WAYLAND_KHR
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numbers>
#include <ranges>
#include <stdexcept>

#include "path.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

uint64_t getTime() {
#ifdef _WIN32
  return GetTickCount64();
#endif

#ifdef __linux__
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (t.tv_sec) * 1000 + (t.tv_nsec / 1000000);
#endif
}

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

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

struct VulkanBuffer {
  VkBuffer buf = VK_NULL_HANDLE;
  VkDeviceMemory mem = VK_NULL_HANDLE;
  void *map = nullptr;

  void cleanup(VkDevice device) {
    if (map != nullptr) {
      vkUnmapMemory(device, mem);
      map = nullptr;
    }
    if (buf != VK_NULL_HANDLE) {
      vkDestroyBuffer(device, buf, nullptr);
      buf = VK_NULL_HANDLE;
    }
    if (mem != VK_NULL_HANDLE) {
      vkFreeMemory(device, mem, nullptr);
      mem = VK_NULL_HANDLE;
    }
  }
};

struct VulkanUniform {
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<VulkanBuffer> buffers;

  void cleanup(VkDevice device) {
    for (auto &buffer : buffers) {
      buffer.cleanup(device);
    }
    if (descriptorPool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(device, descriptorPool, nullptr);
      descriptorPool = VK_NULL_HANDLE;
    }
  }
};

struct VulkanTexture {
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;

  void cleanup(VkDevice device) {
    if (sampler) {
      vkDestroySampler(device, sampler, nullptr);
      sampler = VK_NULL_HANDLE;
    }
    if (view) {
      vkDestroyImageView(device, view, nullptr);
      view = VK_NULL_HANDLE;
    }
    if (image) {
      vkDestroyImage(device, image, nullptr);
      image = VK_NULL_HANDLE;
    }
    if (memory) {
      vkFreeMemory(device, memory, nullptr);
      memory = VK_NULL_HANDLE;
    }
  }
};

struct VulkanPipeline {
  VkPipelineLayout layout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

  void cleanup(VkDevice device) {
    if (pipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(device, pipeline, nullptr);
      pipeline = VK_NULL_HANDLE;
    }
    if (layout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(device, layout, nullptr);
      layout = VK_NULL_HANDLE;
    }
    // if (descriptorPool != VK_NULL_HANDLE) {
    //   vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    //   descriptorPool = VK_NULL_HANDLE;
    // }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
      descriptorSetLayout = VK_NULL_HANDLE;
    }
  }
};

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

VkExtent2D chooseSwapExtent(int windowWidth, int windowHeight,
                            const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    VkExtent2D actualExtent = {static_cast<uint32_t>(windowWidth),
                               static_cast<uint32_t>(windowHeight)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

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

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

VkCommandBuffer createCommandBuffer(VkDevice device,
                                    VkCommandPool commandPool) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
  return commandBuffer;
}

void beginCommand(VkDevice device, VkCommandPool commandPool,
                  VkCommandBuffer commandBuffer) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

void endCommand(VkDevice device, VkCommandPool commandPool,
                VkCommandBuffer commandBuffer, VkQueue queue) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void copyBuffer(VkCommandBuffer commandBuffer,
                VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkBufferCopy copyRegion{.srcOffset = 0, // Optional
                          .dstOffset = 0, // Optional
                          .size = size};
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

VkShaderModule loadShaderFromFile(VkDevice device, const char *shader_name) {
  std::filesystem::path shader_path(root_path());
  shader_path /= "assets";
  shader_path /= "spirv";
  shader_path /= std::string(shader_name) + ".spv";
  std::ifstream ifs(shader_path, std::ios::ate | std::ios::binary);
  if (ifs.fail()) {
    std::cerr << "Error: Failed to open file: " << shader_path << "\n";
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

void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
                           VkFormat format, VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
  VkImageMemoryBarrier barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0, // TODO
      .dstAccessMask = 0, // TODO
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);
}

void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer,
                       VkImage image, uint32_t width, uint32_t height) {
  VkBufferImageCopy region{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .mipLevel = 0,
                           .baseArrayLayer = 0,
                           .layerCount = 1},
      .imageOffset = {0, 0, 0},
      .imageExtent = {width, height, 1}};
  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void recordRenderPass(
    VkRenderPass renderPass,
    const std::vector<VkFramebuffer> &swapChainFramebuffers,
    const VkExtent2D &swapChainExtent, VulkanPipeline pipeline,
    VkDescriptorSet descriptorSet, VkBuffer vertexBuffer,
    VkCommandBuffer commandBuffer, uint32_t imageIndex) {
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
                    pipeline.pipeline);
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

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.layout, 0, 1, &descriptorSet, 0, nullptr);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(commandBuffer);
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  auto *app = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
  app->notifyFramebufferResized(width, height);
  ;
}

void keyEventCallback(GLFWwindow *window, int key, int scancode, int action,
                      int mods) {
  // TODO
}

void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VulkanBuffer &buffer,
                  bool map) {
  VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                .size = size,
                                .usage = usage,
                                .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer.buf) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }
  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer.buf, &memRequirements);
  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = findMemoryType(
          physicalDevice, memRequirements.memoryTypeBits, properties)};
  if (vkAllocateMemory(device, &allocInfo, nullptr, &buffer.mem) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }
  vkBindBufferMemory(device, buffer.buf, buffer.mem, 0);
  if (map) {
    if (vkMapMemory(device, buffer.mem, 0, size, 0, &buffer.map) !=
        VK_SUCCESS) {
      throw std::runtime_error("Could not map buffer memory!");
    }
  }
}

void createDemoPipeline(VkDevice device, const VkExtent2D &swapChainExtent,
                        VkRenderPass renderPass, VulkanPipeline &pipeline) {
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

  VkVertexInputBindingDescription bindingDescription{
      .binding = 0,
      .stride = sizeof(glm::vec3),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
  VkVertexInputAttributeDescription attributeDescription{
      .location = 0,
      .binding = 0,
      .format = VK_FORMAT_R32G32_SFLOAT,
      .offset = 0};
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &bindingDescription,
      .vertexAttributeDescriptionCount = 1,
      .pVertexAttributeDescriptions = &attributeDescription};

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

  VkDescriptorSetLayoutBinding uboLayoutBinding{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .pImmutableSamplers = nullptr};
  VkDescriptorSetLayoutCreateInfo layoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &uboLayoutBinding};
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                  &pipeline.descriptorSetLayout) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &pipeline.descriptorSetLayout,
      .pushConstantRangeCount = 0,     // Optional
      .pPushConstantRanges = nullptr}; // Optional
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipeline.layout) != VK_SUCCESS) {
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
      .layout = pipeline.layout,
      .renderPass = renderPass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = -1};
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &pipeline.pipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, vertShader, nullptr);
  vkDestroyShaderModule(device, fragShader, nullptr);
}

VkRenderPass createRenderPass(VkDevice device, VkFormat swapChainImageFormat) {
  VkAttachmentDescription colorAttachment{
      .format = swapChainImageFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
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
  VkRenderPass renderPass;
  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
  return renderPass;
}

template <typename T>
VulkanUniform createUniform(VkPhysicalDevice physicalDevice, VkDevice device,
                            const VkDescriptorSetLayout &layout,
                            uint32_t count) {
  VulkanUniform uniform;
  VkDeviceSize bufferSize = sizeof(T);
  uniform.buffers.resize(count);
  for (size_t i = 0; i < count; i++) {
    createBuffer(physicalDevice, device, bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniform.buffers[i], /*map=*/true);
  }
  VkDescriptorPoolSize poolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                .descriptorCount = count};
  VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = count,
      .poolSizeCount = 1,
      .pPoolSizes = &poolSize};
  if (vkCreateDescriptorPool(device, &poolInfo, nullptr,
                             &uniform.descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  std::vector<VkDescriptorSetLayout> layouts(count, layout);
  VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = uniform.descriptorPool,
      .descriptorSetCount = count,
      .pSetLayouts = layouts.data()};
  uniform.descriptorSets.resize(count);
  if (vkAllocateDescriptorSets(device, &allocInfo,
                               uniform.descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  for (size_t i = 0; i < count; i++) {
    VkDescriptorBufferInfo bufferInfo{
        .buffer = uniform.buffers[i].buf, .offset = 0, .range = sizeof(T)};
    VkWriteDescriptorSet descriptorWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = uniform.descriptorSets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &bufferInfo,
        .pTexelBufferView = nullptr};
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
  }
  return uniform;
}

void updateUniformBuffer(uint64_t elapsed, VulkanBuffer &buffer) {
  float progress = (elapsed % 4000) / 4000.f;
  float angle = progress * 2 * std::numbers::pi;
  UniformBufferObject ubo{
      glm::rotate(glm::mat4(1.f), angle, glm::vec3(0.f, 0.f, 1.f)),
      glm::mat4(1.f), glm::mat4(1.f)};
  memcpy(buffer.map, &ubo, sizeof(ubo));
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format) {
  VkImageViewCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                     .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                     .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                     .a = VK_COMPONENT_SWIZZLE_IDENTITY},
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};
  VkImageView imageView;
  if (vkCreateImageView(device, &createInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create image view!");
  }

  return imageView;
}

VkSampler createTextureSampler(VkDevice device, float maxAnisotropy) {
  VkSamplerCreateInfo samplerInfo{
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy = maxAnisotropy,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE};
  VkSampler sampler;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
  return sampler;
}

VulkanTexture createTextureImage(VkPhysicalDevice physicalDevice,
                                 VkDevice device, VkCommandPool commandPool,
                                 VkQueue queue, const char *filename) {
  std::filesystem::path texture_path(root_path());
  texture_path /= "assets";
  texture_path /= "textures";
  texture_path /= filename;
  int texWidth, texHeight, texChannels;
  stbi_uc *pixels = stbi_load(path_to_str(texture_path).c_str(), &texWidth,
                              &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  if (!pixels) {
    throw std::runtime_error("failed to load texture image!");
  }

  VulkanTexture texture;
  VulkanBuffer stagingBuffer;
  createBuffer(physicalDevice, device, imageSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, /*map=*/true);
  memcpy(stagingBuffer.map, pixels, imageSize);
  stbi_image_free(pixels);

  VkImageCreateInfo imageInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                              .flags = 0,
                              .imageType = VK_IMAGE_TYPE_2D,
                              .format = VK_FORMAT_R8G8B8A8_SRGB,
                              .extent = {static_cast<uint32_t>(texWidth),
                                         static_cast<uint32_t>(texHeight), 1},
                              .mipLevels = 1,
                              .arrayLayers = 1,
                              .samples = VK_SAMPLE_COUNT_1_BIT,
                              .tiling = VK_IMAGE_TILING_OPTIMAL,
                              .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                       VK_IMAGE_USAGE_SAMPLED_BIT,
                              .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                              .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
  if (vkCreateImage(device, &imageInfo, nullptr, &texture.image) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, texture.image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(physicalDevice, memRequirements.memoryTypeBits,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

  if (vkAllocateMemory(device, &allocInfo, nullptr,
                       &texture.memory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(device, texture.image, texture.memory,
                    0);

  VkCommandBuffer commandBuffer = createCommandBuffer(device, commandPool);
  beginCommand(device, commandPool, commandBuffer);

  transitionImageLayout(commandBuffer, texture.image,
                        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(commandBuffer, stagingBuffer.buf, texture.image,
                    static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight));
  transitionImageLayout(commandBuffer, texture.image,
                        VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  endCommand(device, commandPool, commandBuffer, queue);
  stagingBuffer.cleanup(device);

  texture.view = createImageView(device, texture.image, VK_FORMAT_R8G8B8A8_SRGB);

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  texture.sampler = createTextureSampler(device, properties.limits.maxSamplerAnisotropy);
  return texture;
}

} // namespace

struct VulkanApp::VulkanInnards {
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  uint32_t graphicsQueueNum = std::numeric_limits<uint32_t>::max();
  uint32_t presentationQueueNum = std::numeric_limits<uint32_t>::max();
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentationQueue = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  SwapChainSupportDetails swapChainDetails;
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D swapChainExtent{0, 0};
  VkRenderPass renderPass = VK_NULL_HANDLE;
  VulkanPipeline demoPipeline;
  VulkanBuffer stagingBuffer;
  VulkanBuffer vertexBuffer;
  VulkanTexture statueTexture;
  // VkBuffer stagingBuffer = VK_NULL_HANDLE;
  // VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
  // VkBuffer vertexBuffer = VK_NULL_HANDLE;
  // VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VulkanUniform ubo;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  std::vector<glm::vec3> vertices{
      {0.f, -0.5f, 0.f}, {0.5f, 0.5f, 0.f}, {-0.5f, 0.5f, 0.f}};
};

VulkanApp::VulkanApp() : vi(std::make_unique<VulkanInnards>()) {}

VulkanApp::~VulkanApp() {}

void VulkanApp::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

void VulkanApp::notifyFramebufferResized(int width, int height) {
  windowWidth = width;
  windowHeight = height;
  framebufferResized = true;
}

void VulkanApp::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
  window =
      glfwCreateWindow(windowWidth, windowHeight, "Vulkan", nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  glfwSetKeyCallback(window, keyEventCallback);
}

void VulkanApp::initVulkan() {
  dumpExtensions();
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  vi->renderPass = createRenderPass(vi->device, vi->swapChainImageFormat);
  createGraphicsPipelines();
  createFramebuffers();
  createCommandPool();
  vi->ubo = createUniform<UniformBufferObject>(
      vi->physicalDevice, vi->device, vi->demoPipeline.descriptorSetLayout,
      MAX_FRAMES_IN_FLIGHT);
  createVertexBuffer();
  createCommandBuffers();
  createSyncObjects();
  vi->statueTexture =
      createTextureImage(vi->physicalDevice, vi->device, vi->commandPool,
                         vi->graphicsQueue, "statue.jpg");
}

void VulkanApp::mainLoop() {
  startTicks = getTime();
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }
  vkDeviceWaitIdle(vi->device);
}

void VulkanApp::cleanup() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(vi->device, vi->imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(vi->device, vi->renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(vi->device, vi->inFlightFences[i], nullptr);
  }
  vi->ubo.cleanup(vi->device);
  vkDestroyCommandPool(vi->device, vi->commandPool, nullptr);
  vi->demoPipeline.cleanup(vi->device);
  vkDestroyRenderPass(vi->device, vi->renderPass, nullptr);
  cleanupSwapChain();
  vi->stagingBuffer.cleanup(vi->device);
  vi->vertexBuffer.cleanup(vi->device);
  vi->statueTexture.cleanup(vi->device);
  vkDestroyDevice(vi->device, nullptr);
  vkDestroySurfaceKHR(vi->instance, vi->surface, nullptr);
  vkDestroyInstance(vi->instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void VulkanApp::createInstance() {
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

  VkResult result = vkCreateInstance(&createInfo, nullptr, &vi->instance);
  if (vkCreateInstance(&createInfo, nullptr, &vi->instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

void VulkanApp::createSurface() {
  if (glfwCreateWindowSurface(vi->instance, window, nullptr, &vi->surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("Could not create vulkan surface.");
  }
}

void VulkanApp::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(vi->instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(vi->instance, &deviceCount, devices.data());
  std::vector<std::pair<int, VkPhysicalDevice>> rankedDevices;
  for (const auto &device : devices) {
    rankedDevices.emplace_back(PhysicalDeviceScore(device), device);
  }
  std::sort(rankedDevices.begin(), rankedDevices.end());
  SwapChainSupportDetails details;
  for (const auto &value : rankedDevices | std::views::reverse) {
    auto queues = PhysicalDeviceQueues(value.second, vi->surface);
    bool extensionsOk = CheckPhysicalDeviceExtensions(value.second);
    bool swapChainOk = false;
    if (extensionsOk) {
      details = querySwapChainSupport(value.second, vi->surface);
      swapChainOk = !details.formats.empty() & !details.presentModes.empty();
    }
    if (queues.first.has_value() && queues.second.has_value() && extensionsOk &&
        swapChainOk) {
      vi->physicalDevice = value.second;
      vi->graphicsQueueNum = queues.first.value();
      vi->presentationQueueNum = queues.second.value();
      vi->swapChainDetails = details;
      break;
    }
  }
  if (vi->physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

void VulkanApp::createLogicalDevice() {
  float priority = 1.0;
  VkDeviceQueueCreateInfo queueCreateInfos[2]{
      {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
       .pNext = nullptr,
       .flags = 0,
       .queueFamilyIndex = vi->graphicsQueueNum,
       .queueCount = 1,
       .pQueuePriorities = &priority},
      {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
       .pNext = nullptr,
       .flags = 0,
       .queueFamilyIndex = vi->presentationQueueNum,
       .queueCount = 1,
       .pQueuePriorities = &priority}};
  VkPhysicalDeviceFeatures deviceFeatures{
    .samplerAnisotropy = VK_TRUE
  };
  const char *requiredExtensions[]{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                .queueCreateInfoCount = 2,
                                .pQueueCreateInfos = queueCreateInfos,
                                .enabledLayerCount = 0,
                                .enabledExtensionCount = 1,
                                .ppEnabledExtensionNames = requiredExtensions,
                                .pEnabledFeatures = &deviceFeatures};
  if (vkCreateDevice(vi->physicalDevice, &createInfo, nullptr, &vi->device) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }
  vkGetDeviceQueue(vi->device, vi->graphicsQueueNum, 0, &vi->graphicsQueue);
  vkGetDeviceQueue(vi->device, vi->presentationQueueNum, 0,
                   &vi->presentationQueue);
}

void VulkanApp::createSwapChain() {
  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(vi->swapChainDetails.formats);
  vi->swapChainImageFormat = surfaceFormat.format;
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(vi->swapChainDetails.presentModes);
  vi->swapChainExtent = chooseSwapExtent(windowWidth, windowHeight,
                                         vi->swapChainDetails.capabilities);
  uint32_t imageCount = vi->swapChainDetails.capabilities.minImageCount + 1;
  if (vi->swapChainDetails.capabilities.maxImageCount) {
    imageCount =
        std::min(imageCount, vi->swapChainDetails.capabilities.maxImageCount);
  }
  VkSwapchainCreateInfoKHR createInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = vi->surface,
      .minImageCount = imageCount,
      .imageFormat = vi->swapChainImageFormat,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = vi->swapChainExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform = vi->swapChainDetails.capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE};
  uint32_t queueFamilyIndices[] = {vi->graphicsQueueNum,
                                   vi->presentationQueueNum};
  if (vi->graphicsQueueNum != vi->presentationQueueNum) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }
  if (vkCreateSwapchainKHR(vi->device, &createInfo, nullptr, &vi->swapChain) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }
  vkGetSwapchainImagesKHR(vi->device, vi->swapChain, &imageCount, nullptr);
  vi->swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(vi->device, vi->swapChain, &imageCount,
                          vi->swapChainImages.data());
}

void VulkanApp::cleanupSwapChain() {
  for (auto framebuffer : vi->swapChainFramebuffers) {
    vkDestroyFramebuffer(vi->device, framebuffer, nullptr);
  }
  for (auto imageView : vi->swapChainImageViews) {
    vkDestroyImageView(vi->device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(vi->device, vi->swapChain, nullptr);
}

void VulkanApp::recreateSwapChain() {
  vkDeviceWaitIdle(vi->device);
  cleanupSwapChain();
  createSwapChain();
  createImageViews();
  createFramebuffers();
}

void VulkanApp::createImageViews() {
  vi->swapChainImageViews.resize(vi->swapChainImages.size());
  for (size_t i = 0; i < vi->swapChainImages.size(); ++i) {
    vi->swapChainImageViews[i] =
      createImageView(vi->device, vi->swapChainImages[i], vi->swapChainImageFormat);
  }
}

void VulkanApp::createGraphicsPipelines() {
  createDemoPipeline(vi->device, vi->swapChainExtent, vi->renderPass,
                     vi->demoPipeline);
}

void VulkanApp::createFramebuffers() {
  vi->swapChainFramebuffers.resize(vi->swapChainImageViews.size());
  for (size_t i = 0; i < vi->swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {vi->swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = vi->renderPass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = vi->swapChainExtent.width,
        .height = vi->swapChainExtent.height,
        .layers = 1};

    if (vkCreateFramebuffer(vi->device, &framebufferInfo, nullptr,
                            &vi->swapChainFramebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void VulkanApp::createCommandPool() {
  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = vi->graphicsQueueNum};
  if (vkCreateCommandPool(vi->device, &poolInfo, nullptr, &vi->commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

void VulkanApp::createVertexBuffer() {
  VkDeviceSize bufferSize = sizeof(vi->vertices[0]) * vi->vertices.size();

  createBuffer(vi->physicalDevice, vi->device, bufferSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               vi->stagingBuffer, /*map=*/true);

  createBuffer(
      vi->physicalDevice, vi->device, bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vi->vertexBuffer, /*map=*/false);

  bufferVertexData();
}

void VulkanApp::createCommandBuffers() {
  vi->commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = vi->commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT};

  if (vkAllocateCommandBuffers(vi->device, &allocInfo,
                               vi->commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void VulkanApp::bufferVertexData() {
  VkDeviceSize bufferSize = sizeof(vi->vertices[0]) * vi->vertices.size();
  memcpy(vi->stagingBuffer.map, vi->vertices.data(), (size_t)bufferSize);
  VkCommandBuffer commandBuffer =
      createCommandBuffer(vi->device, vi->commandPool);
  beginCommand(vi->device, vi->commandPool, commandBuffer);
  copyBuffer(commandBuffer,
             vi->stagingBuffer.buf, vi->vertexBuffer.buf, bufferSize);
  endCommand(vi->device, vi->commandPool, commandBuffer, vi->graphicsQueue);
}

void VulkanApp::createSyncObjects() {
  vi->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  vi->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  vi->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (vkCreateSemaphore(vi->device, &semaphoreInfo, nullptr,
                          &vi->imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(vi->device, &semaphoreInfo, nullptr,
                          &vi->renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(vi->device, &fenceInfo, nullptr,
                      &vi->inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores!");
    }
  }
}

void VulkanApp::drawFrame() {
  vkWaitForFences(vi->device, 1, &vi->inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);
  if (framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
  }
  uint64_t nowTicks = getTime();
  updateUniformBuffer(nowTicks, vi->ubo.buffers[currentFrame]);
  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      vi->device, vi->swapChain, UINT64_MAX,
      vi->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }
  vkResetFences(vi->device, 1, &vi->inFlightFences[currentFrame]);
  vkResetCommandBuffer(vi->commandBuffers[currentFrame], 0);
  recordRenderPass(
      vi->renderPass, vi->swapChainFramebuffers, vi->swapChainExtent,
      vi->demoPipeline, vi->ubo.descriptorSets[currentFrame],
      vi->vertexBuffer.buf, vi->commandBuffers[currentFrame], imageIndex);
  VkSemaphore waitSemaphores[] = {vi->imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {vi->renderFinishedSemaphores[currentFrame]};
  VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                          .waitSemaphoreCount = 1,
                          .pWaitSemaphores = waitSemaphores,
                          .pWaitDstStageMask = waitStages,
                          .commandBufferCount = 1,
                          .pCommandBuffers = &vi->commandBuffers[currentFrame],
                          .signalSemaphoreCount = 1,
                          .pSignalSemaphores = signalSemaphores};
  if (vkQueueSubmit(vi->graphicsQueue, 1, &submitInfo,
                    vi->inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }
  VkSwapchainKHR swapChains[] = {vi->swapChain};
  VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                               .waitSemaphoreCount = 1,
                               .pWaitSemaphores = signalSemaphores,
                               .swapchainCount = 1,
                               .pSwapchains = swapChains,
                               .pImageIndices = &imageIndex,
                               .pResults = nullptr};
  result = vkQueuePresentKHR(vi->presentationQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanApp::dumpExtensions() {
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

bool VulkanApp::checkValidationLayerSupport(const char *layerName) {
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
