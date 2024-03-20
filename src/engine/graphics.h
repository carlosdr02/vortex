#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

VkInstance createInstance();

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

    Device() = default;
    Device(VkInstance instance, VkSurfaceKHR surface);
    void destroy();

    VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkSurfaceKHR surface, GLFWwindow* window);
    VkSurfaceFormatKHR getSurfaceFormat(VkSurfaceKHR surface);

    uint32_t getMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties);
};

class Buffer {
public:
    VkDeviceMemory memory;

    Buffer() = default;
    Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
    void destroy(VkDevice device);

    VkDeviceAddress getDeviceAddress(VkDevice device);

    operator VkBuffer();

private:
    VkBuffer buffer;
};

VkRenderPass createRenderPass(VkDevice device, VkFormat format, VkAttachmentLoadOp loadOp);
VkDescriptorPool createGuiDescriptorPool(VkDevice device);

struct RendererCreateInfo {
    VkSurfaceKHR surface;
    const VkSurfaceCapabilitiesKHR* surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkRenderPass renderPass;
    uint32_t framesInFlight;
};

class Renderer {
public:
    Renderer() = default;
    Renderer(Device& device, const RendererCreateInfo& createInfo);
    void destroy(VkDevice device);

    void recordCommandBuffers(VkDevice device);
    bool render(Device& device, VkRenderPass renderPass, VkExtent2D extent);

    void waitIdle(VkDevice device);

    void resize(Device& device, const RendererCreateInfo& createInfo);
    void setFramesInFlight(Device& device, const RendererCreateInfo& createInfo);

private:
    VkSwapchainKHR swapchain;
    VkCommandPool normalCommandPool;
    VkCommandPool transientCommandPool;
    VkDescriptorSetLayout perFrameDescriptorSetLayout;
    uint32_t swapchainImageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkFramebuffer* framebuffers;
    uint32_t framesInFlight;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet* descriptorSets;
    VkCommandBuffer* normalCommandBuffers;
    VkCommandBuffer* transientCommandBuffers;
    VkSemaphore* imageAvailableSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* fences;
    VkImage* offscreenImages;
    VkDeviceMemory offscreenImagesMemory;
    VkImageView* offscreenImageViews;
    uint32_t frameIndex;

    void createSwapchain(VkDevice device, const RendererCreateInfo& createInfo, VkSwapchainKHR oldSwapchain);
    void allocateSwapchainResourcesMemory();
    void createSwapchainResources(VkDevice device, const RendererCreateInfo& createInfo);
    void createFrameResources(VkDevice device);
    void allocateOffscreenResourcesMemory();
    void createOffscreenResources(Device& device, const RendererCreateInfo& createInfo);

    void freeSwapchainResourcesMemory();
    void destroySwapchainResources(VkDevice device);
    void destroyFrameResources(VkDevice device);
    void freeOffscreenResourcesMemory();
    void destroyOffscreenResources(VkDevice device);
};
