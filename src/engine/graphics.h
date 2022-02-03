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
    ~Window();

    operator GLFWwindow*();

private:
    VkInstance instance;
    GLFWwindow* window;
};

class Device {
public:
    VkPhysicalDevice physical;
    uint32_t queueFamilyIndex;
    VkDevice logical;

    Device(VkInstance instance, VkSurfaceKHR surface);
    ~Device();

    VkSurfaceCapabilitiesKHR getSurfaceCapabilities(Window& window);
    VkSurfaceFormatKHR getSurfaceFormat(VkSurfaceKHR surface);
    VkPresentModeKHR getSurfacePresentMode(VkSurfaceKHR surface);
    VkFormat getDepthFormat();
};

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

#endif // !GRAPHICS_H
