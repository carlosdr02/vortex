#ifndef GRAPHICS_H
#define GRAPHICS_H

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

VkInstance createInstance(const char* applicationName, uint32_t applicationVersion);

#ifdef _DEBUG
VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);
#endif // _DEBUG

class Window {
public:
    VkSurfaceKHR surface;

    Window(VkInstance instance, int width, int height, const char* title);
    void destroy(VkInstance instance);

    operator GLFWwindow*();

private:
    GLFWwindow* window;
};

class Device {
public:
    VkPhysicalDevice physical;
    uint32_t queueFamilyIndex;
    VkDevice logical;

    Device(VkInstance instance, VkSurfaceKHR surface);
    void destroy();

    VkSurfaceCapabilitiesKHR getSurfaceCapabilities(Window& window);
    VkSurfaceFormatKHR getSurfaceFormat(VkSurfaceKHR surface);
    VkPresentModeKHR getSurfacePresentMode(VkSurfaceKHR surface);
    VkFormat getDepthStencilFormat();

    VkQueue getQueue(uint32_t queueIndex);

    uint32_t getMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties);
};

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

class Buffer {
public:
    VkDeviceMemory memory;

    void create(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
    void destroy(VkDevice device);

    operator VkBuffer();

private:
    VkBuffer buffer;
};

VkPipelineLayout createPipelineLayout(VkDevice device, uint32_t descriptorSetLayoutCount, const VkDescriptorSetLayout* descriptorSetLayouts);

VkPipeline createComputePipeline(VkDevice device, const char* shaderPath, VkPipelineLayout pipelineLayout);

struct GraphicsPipelineCreateInfo {
    const char* vertexShaderPath;
    const char* fragmentShaderPath;
    const VkPipelineVertexInputStateCreateInfo* vertexInputStateCreateInfo;
    VkPolygonMode polygonMode;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
};

VkPipeline createGraphicsPipeline(VkDevice device, const GraphicsPipelineCreateInfo& createInfo);

struct RendererCreateInfo {
    VkSurfaceKHR surface;
    const VkSurfaceCapabilitiesKHR* surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkFormat depthFormat;
    VkRenderPass renderPass;
    uint32_t framesInFlight;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
};

class Renderer {
public:
    VkDescriptorSetLayout descriptorSetLayout;

    Renderer(Device& device, const RendererCreateInfo& createInfo);
    void recreate(Device& device, const RendererCreateInfo& createInfo);
    void destroy(VkDevice device);

    void recordCommandBuffers(VkDevice device, VkRenderPass renderPass, VkExtent2D extent);
    bool draw(VkDevice device);

    void waitIdle(VkDevice device);

private:
    VkSwapchainKHR swapchain;
    uint32_t swapchainImageCount;
    VkImage* swapchainImages;
    VkImage* depthImages;
    VkDeviceMemory depthImagesMemory;
    VkImageView* swapchainImageViews;
    VkImageView* depthImageViews;
    VkFramebuffer* framebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;
    uint32_t framesInFlight;
    VkSemaphore* imageAvailableSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* imageFences;
    VkFence* frameFences;
    uint32_t imageIndex;
    uint32_t frameIndex;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    void createSwapchainResources(Device& device, const RendererCreateInfo& createInfo);
    void destroySwapchainResources(VkDevice device);
};

#endif // !GRAPHICS_H
