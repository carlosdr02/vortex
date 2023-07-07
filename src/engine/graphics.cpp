#include "graphics.h"

#include <algorithm>

#define COUNT_OF(array) (sizeof(array) / sizeof(array[0]))

static const char* requiredDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
};

VkInstance createInstance() {
    VkApplicationInfo applicationInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = "Achantcraft",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "Vortex",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3
    };

    uint32_t extensionCount;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .pApplicationInfo        = &applicationInfo,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = extensionCount,
        .ppEnabledExtensionNames = extensions
    };

    VkInstance instance;
    vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    return instance;
}

Queue::operator VkQueue() {
    return queue;
}

VkQueue* Queue::operator&() {
    return &queue;
}

static bool isNotDiscrete(VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    return properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

static bool doesNotSupportRequiredExtensions(VkPhysicalDevice physicalDevice) {
    uint32_t extensionPropertyCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);

    VkExtensionProperties* extensionProperties = new VkExtensionProperties[extensionPropertyCount];
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, extensionProperties);

    bool doesNotSupportExtensions = false;

    for (const char* requiredExtension : requiredDeviceExtensions) {
        bool isExtensionAvailable = false;

        for (uint32_t i = 0; i < extensionPropertyCount; ++i) {
            if (strcmp(requiredExtension, extensionProperties[i].extensionName) == 0) {
                isExtensionAvailable = true;
                break;
            }
        }

        if (!isExtensionAvailable) {
            doesNotSupportExtensions = true;
            break;
        }
    }

    delete[] extensionProperties;

    return doesNotSupportExtensions;
}

static void eraseNotSuitablePhysicalDevices(uint32_t& physicalDeviceCount, VkPhysicalDevice* physicalDevices) {
    for (uint32_t i = 0; i < physicalDeviceCount; ) {
        if (isNotDiscrete(physicalDevices[i]) || doesNotSupportRequiredExtensions(physicalDevices[i])) {
            --physicalDeviceCount;

            for (uint32_t j = i; j < physicalDeviceCount; ++j) {
                physicalDevices[j] = physicalDevices[j + 1];
            }
        }
        else {
            ++i;
        }
    }
}

static VkDeviceSize getPhysicalDeviceMemorySize(VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    VkDeviceSize memorySize = 0;

    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
        const VkMemoryHeap& memoryHeap = memoryProperties.memoryHeaps[i];

        if (memoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            memorySize += memoryHeap.size;
        }
    }

    return memorySize;
}

static VkPhysicalDevice getBestPhysicalDevice(uint32_t physicalDeviceCount, const VkPhysicalDevice* physicalDevices) {
    VkDeviceSize maxMemorySize = 0;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    for (uint32_t i = 0; i < physicalDeviceCount; ++i) {
        VkDeviceSize memorySize = getPhysicalDeviceMemorySize(physicalDevices[i]);

        if (memorySize > maxMemorySize) {
            maxMemorySize = memorySize;
            physicalDevice = physicalDevices[i];
        }
    }

    return physicalDevice;
}

Device::Device(VkInstance instance, VkSurfaceKHR surface) {
    // Select a physical device.
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);

    eraseNotSuitablePhysicalDevices(physicalDeviceCount, physicalDevices);

    physical = getBestPhysicalDevice(physicalDeviceCount, physicalDevices);

    delete[] physicalDevices;

    // Select a queue family.
    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyPropertyCount, nullptr);

    VkQueueFamilyProperties* queueFamilyProperties = new VkQueueFamilyProperties[queueFamilyPropertyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyPropertyCount, queueFamilyProperties);

    for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i) {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            VkBool32 surfaceSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &surfaceSupported);

            if (surfaceSupported) {
                renderQueue.familyIndex = i;
                break;
            }
        }
    }

    delete[] queueFamilyProperties;

    // Create the device.
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {
        .sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext                 = nullptr,
        .accelerationStructure = VK_TRUE
    };

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {
        .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext              = &accelerationStructureFeatures,
        .rayTracingPipeline = VK_TRUE
    };

    VkPhysicalDeviceVulkan12Features vulkan12Features = {
        .sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext               = &rayTracingPipelineFeatures,
        .bufferDeviceAddress = VK_TRUE
    };

    VkPhysicalDeviceVulkan13Features vulkan13Features = {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext            = &vulkan12Features,
        .synchronization2 = VK_TRUE
    };

    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = renderQueue.familyIndex,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &vulkan13Features,
        .flags                   = 0,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = COUNT_OF(requiredDeviceExtensions),
        .ppEnabledExtensionNames = requiredDeviceExtensions,
        .pEnabledFeatures        = nullptr
    };

    vkCreateDevice(physical, &deviceCreateInfo, nullptr, &logical);

    // Get the device queue.
    vkGetDeviceQueue(logical, renderQueue.familyIndex, 0, &renderQueue);
}

void Device::destroy() {
    vkDestroyDevice(logical, nullptr);
}

VkSurfaceCapabilitiesKHR Device::getSurfaceCapabilities(VkSurfaceKHR surface, GLFWwindow* window) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &surfaceCapabilities);

    uint32_t maxImageCount = surfaceCapabilities.maxImageCount != 0 ? surfaceCapabilities.maxImageCount : UINT32_MAX;
    surfaceCapabilities.minImageCount = std::clamp(3u, surfaceCapabilities.minImageCount, maxImageCount);

    if (surfaceCapabilities.currentExtent.width == UINT32_MAX) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        surfaceCapabilities.currentExtent = {
            .width  = std::clamp(static_cast<uint32_t>(width), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
            .height = std::clamp(static_cast<uint32_t>(height), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
        };
    }

    return surfaceCapabilities;
}

VkSurfaceFormatKHR Device::getSurfaceFormat(VkSurfaceKHR surface) {
    VkFormat formats[] = {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_SRGB
    };

    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &surfaceFormatCount, nullptr);

    VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &surfaceFormatCount, surfaceFormats);

    VkSurfaceFormatKHR surfaceFormat = {};

    for (VkFormat format : formats) {
        for (uint32_t i = 0; i < surfaceFormatCount; ++i) {
            if (format == surfaceFormats[i].format && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surfaceFormat = surfaceFormats[i];
                goto exit;
            }
        }
    }

exit:
    delete[] surfaceFormats;

    return surfaceFormat;
}

uint32_t Device::getMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties) {
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physical, &physicalDeviceMemoryProperties);

    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
        VkMemoryType memoryType = physicalDeviceMemoryProperties.memoryTypes[i];

        if (memoryTypeBits & (1 << i) && (memoryType.propertyFlags & memoryProperties) == memoryProperties) {
            return i;
        }
    }

    return UINT32_MAX;
}

Buffer::Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties) {
    // Create the buffer.
    VkBufferCreateInfo bufferCreateInfo = {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .size                  = size,
        .usage                 = usage,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr
    };

    vkCreateBuffer(device.logical, &bufferCreateInfo, nullptr, &buffer);

    // Allocate device memory.
    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {
        .sType      = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext      = nullptr,
        .flags      = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0
    };

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device.logical, buffer, &memoryRequirements);

    uint32_t memoryTypeIndex = device.getMemoryTypeIndex(memoryRequirements.memoryTypeBits, memoryProperties);

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT ? &memoryAllocateFlagsInfo : nullptr,
        .allocationSize  = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    vkAllocateMemory(device.logical, &memoryAllocateInfo, nullptr, &memory);

    // Bind the buffer memory.
    vkBindBufferMemory(device.logical, buffer, memory, 0);
}

void Buffer::destroy(VkDevice device) {
    vkFreeMemory(device, memory, nullptr);
    vkDestroyBuffer(device, buffer, nullptr);
}

VkDeviceAddress Buffer::getDeviceAddress(VkDevice device) {
    VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext  = nullptr,
        .buffer = buffer
    };

    return vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);
}

Buffer::operator VkBuffer() {
    return buffer;
}

Renderer::Renderer(Device& device, const RendererCreateInfo& createInfo) {
    createSwapchain(device.logical, createInfo, VK_NULL_HANDLE);
}

void Renderer::destroy(VkDevice device) {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Renderer::resize(Device& device, const RendererCreateInfo& createInfo) {
    // Store the old swapchain.
    VkSwapchainKHR oldSwapchain = swapchain;

    // Create the new swapchain.
    createSwapchain(device.logical, createInfo, oldSwapchain);

    // Destroy the old swapchain.
    vkDestroySwapchainKHR(device.logical, swapchain, nullptr);
}

void Renderer::createSwapchain(VkDevice device, const RendererCreateInfo& createInfo, VkSwapchainKHR oldSwapchain) {
    const VkSurfaceCapabilitiesKHR* surfaceCapabilities = createInfo.surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat = createInfo.surfaceFormat;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                 = nullptr,
        .flags                 = 0,
        .surface               = createInfo.surface,
        .minImageCount         = surfaceCapabilities->minImageCount,
        .imageFormat           = surfaceFormat.format,
        .imageColorSpace       = surfaceFormat.colorSpace,
        .imageExtent           = surfaceCapabilities->currentExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = surfaceCapabilities->currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = VK_PRESENT_MODE_FIFO_KHR, // TODO
        .clipped               = VK_TRUE,
        .oldSwapchain          = oldSwapchain
    };

    vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
}
