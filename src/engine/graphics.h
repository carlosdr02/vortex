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

struct RendererCreateInfo {
    VkSurfaceKHR surface;
    const VkSurfaceCapabilitiesKHR* surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    uint32_t framesInFlight;
};

class Renderer {
public:
    Renderer() = default;
    Renderer(Device& device, const RendererCreateInfo& createInfo);
    void recreate(Device& device, const RendererCreateInfo& createInfo);
    void destroy(VkDevice device);

    void recordCommandBuffers(VkDevice device, VkExtent2D extent);

private:
    VkSwapchainKHR swapchain;
    VkCommandPool commandPool;

    uint32_t framesInFlight;
    VkSemaphore* imageAcquiredSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* fences;

    uint32_t swapchainImageCount;
    VkImage* swapchainImages;

    VkCommandBuffer* commandBuffers;

    VkImage* storageImages;
    VkDeviceMemory storageImagesMemory;
    VkImageView* storageImageViews;

    void createSwapchain(VkDevice device, const RendererCreateInfo& createInfo, VkSwapchainKHR oldSwapchain);
    void createSwapchainResources(Device& device, const RendererCreateInfo& createInfo);
    void destroySwapchainResources(VkDevice device);
};