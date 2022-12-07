#ifndef GRAPHICS_H
#define GRAPHICS_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

VkInstance createInstance(const char* applicationName, uint32_t applicationVersion);

class Queue {
public:
    uint32_t familyIndex;

    operator VkQueue();
    VkQueue* operator&();

private:
    VkQueue queue;
};

class Device {
public:
    VkPhysicalDevice physical;
    Queue renderQueue;
    VkDevice logical;

    Device(VkInstance instance, VkSurfaceKHR surface);
    void destroy();

    VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkSurfaceKHR surface, GLFWwindow* window);
    VkSurfaceFormatKHR getSurfaceFormat(VkSurfaceKHR surface);
    VkPresentModeKHR getSurfacePresentMode(VkSurfaceKHR surface);
    VkFormat getDepthFormat();

    uint32_t getMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties);
};

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

class Buffer {
public:
    VkDeviceMemory memory;

    Buffer() = default;
    Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
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
    VkDeviceSize cameraDataSize;
    uint32_t framesInFlight;
};

class Renderer {
public:
    VkDescriptorSetLayout descriptorSetLayout;

    Renderer(Device& device, const RendererCreateInfo& createInfo);
    void recreate(Device& device, const RendererCreateInfo& createInfo);
    void destroy(VkDevice device);

    void recordCommandBuffers(VkDevice device, VkRenderPass renderPass, VkExtent2D extent);
    bool draw(Device& device, const void* cameraData);

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
    VkDeviceSize cameraDataSize;
    Buffer uniformBuffer;
    void* mappedUniformBufferMemory;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet* descriptorSets;
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;
    uint32_t framesInFlight;
    VkSemaphore* imageAvailableSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* imageFences;
    VkFence* frameFences;
    uint32_t imageIndex;
    uint32_t frameIndex;

    void createSwapchain(VkDevice device, const RendererCreateInfo& createInfo, VkSwapchainKHR oldSwapchain);
    void createSwapchainResources(Device& device, const RendererCreateInfo& createInfo);
    void destroySwapchainResources(VkDevice device);
};

#endif // !GRAPHICS_H
