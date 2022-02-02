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
    void destroy();

    operator GLFWwindow*();

private:
    VkInstance instance;
    GLFWwindow* window;
};

class Device {
public:
    VkPhysicalDevice physical;
    VkPhysicalDeviceMemoryProperties2 memoryProperties;
    uint32_t queueFamilyIndex;
    VkDevice logical;

    Device(VkInstance instance, Window& window);
    void destroy();

    VkSurfaceFormatKHR getSurfaceFormat(Window& window);
    VkPresentModeKHR getSurfacePresentMode(Window& window);
    VkFormat getDepthFormat();
};

VkRenderPass createRenderPass(Device& device, VkFormat format);

#endif // !GRAPHICS_H
