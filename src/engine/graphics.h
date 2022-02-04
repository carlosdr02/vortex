#ifndef GRAPHICS_H
#define GRAPHICS_H

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
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
};

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

struct RendererCreateInfo {
    VkSurfaceKHR surface;
    const VkSurfaceCapabilitiesKHR* surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
};

class Renderer {
public:
    Renderer(Device& device, const RendererCreateInfo& createInfo, Renderer* oldRenderer);
    void destroy(VkDevice device);

private:
    VkSwapchainKHR swapchain;
    uint32_t swapchainImageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
};

#endif // !GRAPHICS_H
