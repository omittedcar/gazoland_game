#include <cstdint>
#include <memory>

struct GLFWwindow;

class VulkanApp {
public:
  VulkanApp();
  ~VulkanApp();

  void run();
  void notifyFramebufferResized(int width, int height);

private:
  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();
  void createInstance();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapChain();
  void cleanupSwapChain();
  void recreateSwapChain();
  void createImageViews();
  void createDescriptorSetLayout();
  void createGraphicsPipelines();
  void createFramebuffers();
  void createCommandPool();
  void createUniformBuffers();
  void createVertexBuffer();
  void createIndexBuffer();
  void createCommandBuffers();
  void createSyncObjects();
  void drawFrame();
  void dumpExtensions();
  bool checkValidationLayerSupport(const char *layerName);

  const uint32_t MAX_FRAMES_IN_FLIGHT = 2u;
  uint64_t startTicks;
  GLFWwindow *window = nullptr;
  int windowWidth = 800;
  int windowHeight = 600;
  size_t currentFrame = 0;
  bool framebufferResized = false;

  struct VulkanInnards;
  std::unique_ptr<VulkanInnards> vi;
};