#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <imgui_impl_vulkan.h>

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
};

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat);

VkDescriptorPool createGuiDescriptorPool(VkDevice device);
void createGuiFonts(Device& device);

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
    void recreate(Device& device, const RendererCreateInfo& createInfo);
    void destroy(VkDevice device);

    bool render(Device& device, VkRenderPass renderPass, VkExtent2D extent, ImDrawData* drawData);

    void waitIdle(VkDevice device);

private:
    VkSwapchainKHR swapchain;
    VkCommandPool commandPool;
    uint32_t swapchainImageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkFramebuffer* framebuffers;
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
