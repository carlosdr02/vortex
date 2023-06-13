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

VkRenderPass createRenderPass(VkDevice device);

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

VkPipelineLayout createPipelineLayout(VkDevice device, uint32_t descriptorSetLayoutCount, const VkDescriptorSetLayout* descriptorSetLayouts);

struct RendererCreateInfo {
    VkSurfaceKHR surface;
    const VkSurfaceCapabilitiesKHR* surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkRenderPass renderPass;
    uint32_t framesInFlight;
    VkDeviceSize uniformDataSize;
};

class Renderer {
public:
    Renderer() = default;
    Renderer(Device& device, const RendererCreateInfo& createInfo);
    void recreate(Device& device, const RendererCreateInfo& createInfo);
    void destroy(VkDevice device);

    void recordCommandBuffers(VkDevice device, VkRenderPass renderPass, VkExtent2D extent);
    bool render(Device& device, VkExtent2D extent, const void* uniformData);

    void waitIdle(VkDevice device);

private:
    VkSwapchainKHR swapchain;
    VkCommandPool framesCommandPool;
    VkCommandPool imagesCommandPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;

    uint32_t swapchainImageCount;
    VkImage* swapchainImages;

    uint32_t framesInFlight;

    VkCommandBuffer* frameCommandBuffers;
    VkCommandBuffer* imageCommandBuffers;

    VkImage* offscreenImages;
    VkDeviceMemory offscreenImagesMemory;
    VkImageView* offscreenImageViews;

    VkFramebuffer* framebuffers;

    Buffer uniformBuffer;
    VkDeviceSize uniformDataSize;
    void* uniformBufferData;

    VkDescriptorSet* descriptorSets;

    VkSemaphore* imageAcquiredSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* frameFences;
    VkFence* imageFences;

    uint32_t frameIndex = 0;

    void createSwapchainResources(Device& device, const RendererCreateInfo& createInfo);
    void destroySwapchainResources(VkDevice device);
};
