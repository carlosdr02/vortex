#include "graphics.h"

#include <cstring>

#include <algorithm>
#include <array>
#include <vector>

#define COUNT_OF(array) (sizeof(array) / sizeof(array[0]))

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

static std::array<const char*, 4> getRequiredDeviceExtensions() {
    std::array<const char*, 4> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };

    return deviceExtensions;
}

static bool doesNotSupportRequiredExtensions(VkPhysicalDevice physicalDevice) {
    std::array<const char*, 4> requiredDeviceExtensions = getRequiredDeviceExtensions();

    uint32_t extensionPropertyCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);

    VkExtensionProperties* extensionProperties = new VkExtensionProperties[extensionPropertyCount];
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, extensionProperties);

    bool doesNotSupportExtensions = true;

    for (const char* requiredExtension : requiredDeviceExtensions) {
        bool isExtensionAvailable = false;

        for (uint32_t j = 0; j < extensionPropertyCount; ++j) {
            if (strcmp(requiredExtension, extensionProperties[j].extensionName) == 0) {
                isExtensionAvailable = true;
                break;
            }
        }

        if (!isExtensionAvailable) {
            doesNotSupportExtensions = false;
            break;
        }
    }

    delete[] extensionProperties;

    return doesNotSupportExtensions;
}

static std::vector<VkPhysicalDevice> getDiscretePhysicalDevices(VkInstance instance) {
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

    std::erase_if(physicalDevices, isNotDiscrete);
    std::erase_if(physicalDevices, doesNotSupportRequiredExtensions);

    return physicalDevices;
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

Device::Device(VkInstance instance, VkSurfaceKHR surface) {
    // Select a physical device.
    std::vector<VkPhysicalDevice> physicalDevices = getDiscretePhysicalDevices(instance);
    physical = *std::ranges::max_element(physicalDevices, {}, getPhysicalDeviceMemorySize);

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
    VkPhysicalDeviceVulkan13Features enabledFeatures = {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext            = nullptr,
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

    std::array<const char*, 4> deviceExtensions = getRequiredDeviceExtensions();

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &enabledFeatures,
        .flags                   = 0,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = deviceExtensions.size(),
        .ppEnabledExtensionNames = deviceExtensions.data(),
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

    VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];

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

Renderer::Renderer(Device& device, const RendererCreateInfo& createInfo) {
    // Create the swapchain.
    createSwapchain(device.logical, createInfo, VK_NULL_HANDLE);

    // Create the command pool.
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = device.renderQueue.familyIndex
    };

    vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &commandPool);

    // Create the swapchain resources.
    createSwapchainResources(device, createInfo);
}

void Renderer::recreate(Device& device, const RendererCreateInfo& createInfo) {
    // Destroy the old swapchain resources.
    destroySwapchainResources(device.logical);

    // Store the old swapchain.
    VkSwapchainKHR oldSwapchain = swapchain;

    // Create the new swapchain.
    createSwapchain(device.logical, createInfo, oldSwapchain);

    // Destroy the old swapchain.
    vkDestroySwapchainKHR(device.logical, oldSwapchain, nullptr);

    // Create the new swapchain resources.
    createSwapchainResources(device, createInfo);
}

void Renderer::destroy(VkDevice device) {
    destroySwapchainResources(device);

    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Renderer::createSwapchain(VkDevice device, const RendererCreateInfo& createInfo, VkSwapchainKHR oldSwapchain) {
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                 = nullptr,
        .flags                 = 0,
        .surface               = createInfo.surface,
        .minImageCount         = createInfo.surfaceCapabilities.minImageCount,
        .imageFormat           = createInfo.surfaceFormat.format,
        .imageColorSpace       = createInfo.surfaceFormat.colorSpace,
        .imageExtent           = createInfo.surfaceCapabilities.currentExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = createInfo.surfaceCapabilities.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = VK_PRESENT_MODE_FIFO_KHR, // TODO
        .clipped               = VK_TRUE,
        .oldSwapchain          = oldSwapchain
    };

    vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
}

void Renderer::createSwapchainResources(Device& device, const RendererCreateInfo& createInfo) {
    // Get the swapchain images.
    vkGetSwapchainImagesKHR(device.logical, swapchain, &swapchainImageCount, nullptr);

    swapchainImages = new VkImage[swapchainImageCount];
    vkGetSwapchainImagesKHR(device.logical, swapchain, &swapchainImageCount, swapchainImages);

    // Create the storage images.
    storageImages = new VkImage[swapchainImageCount];

    VkExtent2D extent = createInfo.surfaceCapabilities.currentExtent;

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        VkImageCreateInfo imageCreateInfo = {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = 0,
            .imageType             = VK_IMAGE_TYPE_2D,
            .format                = VK_FORMAT_R16G16B16A16_SFLOAT,
            .extent                = { extent.width, extent.height, 1 },
            .mipLevels             = 1,
            .arrayLayers           = 1,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices   = nullptr,
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED
        };

        vkCreateImage(device.logical, &imageCreateInfo, nullptr, &storageImages[i]);
    }

    // Allocate device memory.
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device.logical, storageImages[0], &memoryRequirements);

    uint32_t memoryTypeIndex = device.getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = nullptr,
        .allocationSize  = swapchainImageCount * memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    vkAllocateMemory(device.logical, &memoryAllocateInfo, nullptr, &storageImagesMemory);

    VkBindImageMemoryInfo* bindImageMemoryInfos = new VkBindImageMemoryInfo[swapchainImageCount];

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        bindImageMemoryInfos[i].sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
        bindImageMemoryInfos[i].pNext = nullptr;
        bindImageMemoryInfos[i].image = storageImages[i];
        bindImageMemoryInfos[i].memory = storageImagesMemory;
        bindImageMemoryInfos[i].memoryOffset = i * memoryRequirements.size;
    }

    vkBindImageMemory2(device.logical, swapchainImageCount, bindImageMemoryInfos);

    delete[] bindImageMemoryInfos;

    // Create the storage image views.
    storageImageViews = new VkImageView[swapchainImageCount];

    VkImageSubresourceRange imageSubresourceRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    };

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .image            = storageImages[i],
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = VK_FORMAT_R16G16B16A16_SFLOAT,
            .components       = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = imageSubresourceRange
        };

        vkCreateImageView(device.logical, &imageViewCreateInfo, nullptr, &storageImageViews[i]);
    }

    // Allocate the command buffers.
    commandBuffers = new VkCommandBuffer[swapchainImageCount];

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = swapchainImageCount
    };

    vkAllocateCommandBuffers(device.logical, &commandBufferAllocateInfo, commandBuffers);

    // Set the storage image layouts.
    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(commandBuffers[0], &commandBufferBeginInfo);

    VkImageMemoryBarrier2* imageMemoryBarriers = new VkImageMemoryBarrier2[swapchainImageCount];

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        imageMemoryBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageMemoryBarriers[i].pNext = nullptr;
        imageMemoryBarriers[i].srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        imageMemoryBarriers[i].srcAccessMask = VK_ACCESS_2_NONE;
        imageMemoryBarriers[i].dstStageMask = VK_PIPELINE_STAGE_2_NONE;
        imageMemoryBarriers[i].dstAccessMask = VK_ACCESS_2_NONE;
        imageMemoryBarriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarriers[i].image = storageImages[i];
        imageMemoryBarriers[i].subresourceRange = imageSubresourceRange;
    }

    VkDependencyInfo dependencyInfo = {
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext                    = nullptr,
        .dependencyFlags          = 0,
        .memoryBarrierCount       = 0,
        .pMemoryBarriers          = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers    = nullptr,
        .imageMemoryBarrierCount  = swapchainImageCount,
        .pImageMemoryBarriers     = imageMemoryBarriers
    };

    vkCmdPipelineBarrier2(commandBuffers[0], &dependencyInfo);

    delete[] imageMemoryBarriers;

    vkEndCommandBuffer(commandBuffers[0]);

    VkSubmitInfo submitInfo = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = nullptr,
        .waitSemaphoreCount   = 0,
        .pWaitSemaphores      = nullptr,
        .pWaitDstStageMask    = nullptr,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &commandBuffers[0],
        .signalSemaphoreCount = 0,
        .pSignalSemaphores    = nullptr
    };

    vkQueueSubmit(device.renderQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device.renderQueue);
}

void Renderer::destroySwapchainResources(VkDevice device) {
    vkFreeCommandBuffers(device, commandPool, swapchainImageCount, commandBuffers);

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        vkDestroyImageView(device, storageImageViews[i], nullptr);
    }

    vkFreeMemory(device, storageImagesMemory, nullptr);

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        vkDestroyImage(device, storageImages[i], nullptr);
    }

    delete[] commandBuffers;
    delete[] storageImageViews;
    delete[] storageImages;
    delete[] swapchainImages;
}