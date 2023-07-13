#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Creates an instance with no validation layers, the VK_KHR_surface extension
// and its corresponding VK_KHR_*platform*_surface platform-specific extension.
VkInstance createInstance();

class Queue {
public:
    // That's right, no getters and setters, go back to Java ;)
    uint32_t familyIndex;

    // Convenience conversion operator, just so that we don't have to do
    // queue.queue when accessing the VkQueue handle. The same is done for the
    // Buffer class.
    operator VkQueue();
    VkQueue* operator&();

private:
    VkQueue queue;
};

class Device {
public:
    // If its the first time executing the program, or the user hasn't selected
    // a specific GPU, we will use the discrete GPU with most memory, that
    // supports the VK_KHR_ray_tracing_pipeline extension.
    VkPhysicalDevice physical;

    // Outside the Renderer::render method, this queue should only be used to
    // transfer exclusive ownership from and to queues from a different queue
    // family.
    Queue renderQueue;
    VkDevice logical;

    Device() = default;
    Device(VkInstance instance, VkSurfaceKHR surface);
    void destroy();

    VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkSurfaceKHR surface, GLFWwindow* window);

    // Returns the first available four-component, 32-bit unsigned normalized,
    // sRGB nonlinear format.
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

VkRenderPass createRenderPass(VkDevice device, VkFormat format);

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

    // Returns true if vkAcquireNextImageKHR returns VK_ERROR_OUT_OF_DATE_KHR,
    // which means we should resize via the resize method, otherwise false.
    bool render(Device& device, VkRenderPass renderPass, VkExtent2D extent);

    void waitIdle(VkDevice device);

    void resize(VkDevice device, const RendererCreateInfo& createInfo);
    void setFramesInFlight(VkDevice device, uint32_t framesInFlight);

private:
    VkSwapchainKHR swapchain;
    VkCommandPool commandPool;
    uint32_t swapchainImageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkFramebuffer* framebuffers;
    uint32_t framesInFlight;
    VkCommandBuffer* commandBuffers;
    VkSemaphore* imageAvailableSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* fences;
    uint32_t frameIndex;

    void createSwapchain(VkDevice device, const RendererCreateInfo& createInfo, VkSwapchainKHR oldSwapchain);
    void allocateSwapchainResourcesHostMemory();
    void createSwapchainResources(VkDevice device, const RendererCreateInfo& createInfo);
    void createFrameResources(VkDevice device);

    void freeSwapchainResourcesHostMemory();
    void destroySwapchainResources(VkDevice device);
    void destroyFrameResources(VkDevice device);
};
