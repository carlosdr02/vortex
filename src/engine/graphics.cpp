#include "graphics.h"

#define COUNT_OF(array) (sizeof(array) / sizeof(array[0]))

#ifdef _DEBUG
#include <stdio.h>

static VkBool32 debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    printf("Debug messenger: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = nullptr,
        .flags           = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugMessengerCallback,
        .pUserData       = nullptr
    };

    return debugMessengerCreateInfo;
}
#endif // _DEBUG

VkInstance createInstance(const char* applicationName, uint32_t applicationVersion) {
    VkApplicationInfo applicationInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = applicationName,
        .applicationVersion = applicationVersion,
        .pEngineName        = "Vortex",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_2
    };

#ifdef _DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = getDebugMessengerCreateInfo();

    const char* validationLayer = "VK_LAYER_KHRONOS_validation";

    const char* extensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = &debugMessengerCreateInfo,
        .flags                   = 0,
        .pApplicationInfo        = &applicationInfo,
        .enabledLayerCount       = 1,
        .ppEnabledLayerNames     = &validationLayer,
        .enabledExtensionCount   = COUNT_OF(extensions),
        .ppEnabledExtensionNames = extensions
    };
#else

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .pApplicationInfo        = &applicationInfo,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = COUNT_OF(extensions),
        .ppEnabledExtensionNames = extensions
    };
#endif // _DEBUG

    VkInstance instance;
    vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    return instance;
}

#ifdef _DEBUG
VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) {
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = getDebugMessengerCreateInfo();

    VkDebugUtilsMessengerEXT debugMessenger;
    vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger);

    return debugMessenger;
}

void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) {
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
}
#endif // _DEBUG

Window::Window(VkInstance instance, int width, int height, const char* title) : instance(instance) {
    // Create the window.
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    // Create the surface.
    glfwCreateWindowSurface(instance, window, nullptr, &surface);
}

void Window::destroy() {
    // Destroy the surface.
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // Destroy the window.
    glfwDestroyWindow(window);
}

Window::operator GLFWwindow*() {
    return window;
}

static VkDeviceSize getMaxDeviceLocalHeapSize(const VkPhysicalDeviceMemoryProperties2& memoryProperties) {
    VkDeviceSize maxDeviceLocalHeapSize = 0;

    for (const VkMemoryHeap& heap : memoryProperties.memoryProperties.memoryHeaps) {
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT && heap.size > maxDeviceLocalHeapSize) {
            maxDeviceLocalHeapSize = heap.size;
        }
    }

    return maxDeviceLocalHeapSize;
}

static uint32_t getMaxSizeIndex(uint32_t sizeCount, const VkDeviceSize* sizes) {
    VkDeviceSize max = 0;
    uint32_t index = 0;

    for (uint32_t i = 0; i < sizeCount; ++i) {
        if (sizes[i] > max) {
            max = sizes[i];
            index = i;
        }
    }

    return index;
}

Device::Device(VkInstance instance) {
    memoryProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };

    // Select a physical device.
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);

    VkDeviceSize* deviceLocalHeapSizes = new VkDeviceSize[physicalDeviceCount]();

    for (uint32_t i = 0; i < physicalDeviceCount; ++i) {
        VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        vkGetPhysicalDeviceProperties2(physicalDevices[i], &properties);

        if (properties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            continue;
        }

        VkPhysicalDeviceMemoryProperties2 memoryProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
        vkGetPhysicalDeviceMemoryProperties2(physicalDevices[i], &memoryProperties);

        deviceLocalHeapSizes[i] = getMaxDeviceLocalHeapSize(memoryProperties);
    }

    uint32_t index = getMaxSizeIndex(physicalDeviceCount, deviceLocalHeapSizes);
    physical = physicalDevices[index];

    vkGetPhysicalDeviceMemoryProperties2(physical, &memoryProperties);

    delete[] physicalDevices;
    delete[] deviceLocalHeapSizes;
}
