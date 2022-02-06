#include "graphics.h"

#include <stdint.h>

#include <algorithm>

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

Window::Window(VkInstance instance, int width, int height, const char* title) {
    // Create the window.
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    // Create the surface.
    glfwCreateWindowSurface(instance, window, nullptr, &surface);
}

void Window::destroy(VkInstance instance) {
    vkDestroySurfaceKHR(instance, surface, nullptr);

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

Device::Device(VkInstance instance, VkSurfaceKHR surface) {
    // Select a physical device.
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);

    VkDeviceSize* deviceLocalHeapSizes = new VkDeviceSize[physicalDeviceCount]();

    for (uint32_t i = 0; i < physicalDeviceCount; ++i) {
        VkPhysicalDeviceProperties2 physicalDeviceProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        vkGetPhysicalDeviceProperties2(physicalDevices[i], &physicalDeviceProperties);

        if (physicalDeviceProperties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            continue;
        }

        VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
        vkGetPhysicalDeviceMemoryProperties2(physicalDevices[i], &physicalDeviceMemoryProperties);

        deviceLocalHeapSizes[i] = getMaxDeviceLocalHeapSize(physicalDeviceMemoryProperties);
    }

    uint32_t index = std::max_element(deviceLocalHeapSizes, deviceLocalHeapSizes + physicalDeviceCount) - deviceLocalHeapSizes;
    physical = physicalDevices[index];

    delete[] deviceLocalHeapSizes;
    delete[] physicalDevices;

    // Select a queue family.
    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyPropertyCount, nullptr);

    VkQueueFamilyProperties* queueFamilyProperties = new VkQueueFamilyProperties[queueFamilyPropertyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyPropertyCount, queueFamilyProperties);

    for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i) {
        uint32_t queueCount = queueFamilyProperties[i].queueCount;

        if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 || queueCount < 2) {
            continue;
        }

        VkBool32 surfaceSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &surfaceSupported);

        if (surfaceSupported) {
            queueFamilyIndex = i;
            break;
        }
    }

    delete[] queueFamilyProperties;

    // Create the device.
    float queuePriorities[] = {
        1.0f, 1.0f
    };

    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount       = COUNT_OF(queuePriorities),
        .pQueuePriorities = queuePriorities
    };

    const char* swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &queueCreateInfo,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = 1,
        .ppEnabledExtensionNames = &swapchainExtension,
        .pEnabledFeatures        = nullptr
    };

    vkCreateDevice(physical, &deviceCreateInfo, nullptr, &logical);
}

void Device::destroy() {
    vkDestroyDevice(logical, nullptr);
}

VkSurfaceCapabilitiesKHR Device::getSurfaceCapabilities(Window& window) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, window.surface, &surfaceCapabilities);

    uint32_t& minImageCount = surfaceCapabilities.minImageCount;
    uint32_t maxImageCount = surfaceCapabilities.maxImageCount;

    if (minImageCount < 3 && (maxImageCount > minImageCount || maxImageCount == 0)) {
        ++minImageCount;
    }

    VkExtent2D& extent = surfaceCapabilities.currentExtent;

    if (extent.width == UINT32_MAX) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D minExtent = surfaceCapabilities.minImageExtent;
        VkExtent2D maxExtent = surfaceCapabilities.maxImageExtent;

        extent.width = std::clamp((uint32_t)width, minExtent.width, maxExtent.width);
        extent.height = std::clamp((uint32_t)height, minExtent.height, maxExtent.height);
    }

    return surfaceCapabilities;
}

VkSurfaceFormatKHR Device::getSurfaceFormat(VkSurfaceKHR surface) {
    VkFormat prefferedFormats[] = {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM
    };

    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &surfaceFormatCount, nullptr);

    VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &surfaceFormatCount, surfaceFormats);

    VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];

    for (VkFormat prefferedFormat : prefferedFormats) {
        for (uint32_t i = 0; i < surfaceFormatCount; ++i) {
            if (surfaceFormats[i].format == prefferedFormat) {
                surfaceFormat = surfaceFormats[i];
                goto exit;
            }
        }
    }

exit:
    delete[] surfaceFormats;

    return surfaceFormat;
}

VkPresentModeKHR Device::getSurfacePresentMode(VkSurfaceKHR surface) {
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &presentModeCount, nullptr);

    VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &presentModeCount, presentModes);

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    for (uint32_t i = 0; i < presentModeCount; ++i) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    delete[] presentModes;

    return presentMode;
}

VkFormat Device::getDepthStencilFormat() {
    VkFormat prefferedDepthStencilFormats[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT
    };

    for (VkFormat prefferedDepthStencilFormat : prefferedDepthStencilFormats) {
        VkFormatProperties2 formatProperties = { VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
        vkGetPhysicalDeviceFormatProperties2(physical, prefferedDepthStencilFormat, &formatProperties);

        if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return prefferedDepthStencilFormat;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

uint32_t Device::getMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties) {
    VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
    vkGetPhysicalDeviceMemoryProperties2(physical, &physicalDeviceMemoryProperties);

    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryProperties.memoryTypeCount; ++i) {
        VkMemoryType memoryType = physicalDeviceMemoryProperties.memoryProperties.memoryTypes[i];

        if (memoryTypeBits & (1 << i) && (memoryType.propertyFlags & memoryProperties) == memoryProperties) {
            return i;
        }
    }

    return UINT32_MAX;
}

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat) {
    VkAttachmentDescription2 colorAttachmentDescription = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .pNext          = nullptr,
        .flags          = 0,
        .format         = colorFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentDescription2 depthAttachmentDescription = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .pNext          = nullptr,
        .flags          = 0,
        .format         = depthFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription2 attachmentDescriptions[] = {
        colorAttachmentDescription,
        depthAttachmentDescription
    };

    VkAttachmentReference2 colorAttachmentReference = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .pNext      = nullptr,
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .aspectMask = 0
    };

    VkAttachmentReference2 depthAttachmentReference = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .pNext      = nullptr,
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .aspectMask = 0
    };

    VkSubpassDescription2 subpassDescription = {
        .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
        .pNext                   = nullptr,
        .flags                   = 0,
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .viewMask                = 0,
        .inputAttachmentCount    = 0,
        .pInputAttachments       = nullptr,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &colorAttachmentReference,
        .pResolveAttachments     = nullptr,
        .pDepthStencilAttachment = &depthAttachmentReference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments    = nullptr
    };

    VkSubpassDependency2 subpassDependency = {
        .sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
        .pNext           = nullptr,
        .srcSubpass      = VK_SUBPASS_EXTERNAL,
        .dstSubpass      = 0,
        .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask   = 0,
        .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        .viewOffset      = 0
    };

    VkRenderPassCreateInfo2 renderPassCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .pNext                   = nullptr,
        .flags                   = 0,
        .attachmentCount         = COUNT_OF(attachmentDescriptions),
        .pAttachments            = attachmentDescriptions,
        .subpassCount            = 1,
        .pSubpasses              = &subpassDescription,
        .dependencyCount         = 1,
        .pDependencies           = &subpassDependency,
        .correlatedViewMaskCount = 0,
        .pCorrelatedViewMasks    = nullptr
    };

    VkRenderPass renderPass;
    vkCreateRenderPass2(device, &renderPassCreateInfo, nullptr, &renderPass);

    return renderPass;
}

Renderer::Renderer(Device& device, const RendererCreateInfo& createInfo, Renderer* oldRenderer) : framesInFlight(createInfo.framesInFlight), frameIndex(0) {
    // Create the swapchain.
    const VkSurfaceCapabilitiesKHR* surfaceCapabilities = createInfo.surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat = createInfo.surfaceFormat;
    VkExtent2D extent = surfaceCapabilities->currentExtent;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                 = nullptr,
        .flags                 = 0,
        .surface               = createInfo.surface,
        .minImageCount         = surfaceCapabilities->minImageCount,
        .imageFormat           = surfaceFormat.format,
        .imageColorSpace       = surfaceFormat.colorSpace,
        .imageExtent           = extent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = surfaceCapabilities->currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = createInfo.presentMode,
        .clipped               = VK_TRUE
    };

    swapchainCreateInfo.oldSwapchain = oldRenderer ? oldRenderer->swapchain : VK_NULL_HANDLE;

    vkCreateSwapchainKHR(device.logical, &swapchainCreateInfo, nullptr, &swapchain);

    // Get the swapchain image count.
    vkGetSwapchainImagesKHR(device.logical, swapchain, &swapchainImageCount, nullptr);

    // Allocate host memory.
    swapchainImages = new VkImage[swapchainImageCount];
    depthImages = new VkImage[swapchainImageCount];
    swapchainImageViews = new VkImageView[swapchainImageCount];
    depthImageViews = new VkImageView[swapchainImageCount];
    framebuffers = new VkFramebuffer[swapchainImageCount];
    commandBuffers = new VkCommandBuffer[swapchainImageCount];
    imageAvailableSemaphores = new VkSemaphore[framesInFlight];
    renderFinishedSemaphores = new VkSemaphore[framesInFlight];
    imageFences = new VkFence[swapchainImageCount];
    frameFences = new VkFence[framesInFlight];

    // Get the swapchain images.
    vkGetSwapchainImagesKHR(device.logical, swapchain, &swapchainImageCount, swapchainImages);

    // Create the depth images.
    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        VkImageCreateInfo depthImageCreateInfo = {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = 0,
            .imageType             = VK_IMAGE_TYPE_2D,
            .format                = createInfo.depthFormat,
            .extent                = { extent.width, extent.height, 1 },
            .mipLevels             = 1,
            .arrayLayers           = 1,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices   = nullptr,
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED
        };

        vkCreateImage(device.logical, &depthImageCreateInfo, nullptr, &depthImages[i]);
    }

    // Allocate device memory.
    VkImageMemoryRequirementsInfo2 depthImagesMemoryRequirementsInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
        .pNext = nullptr,
        .image = depthImages[0]
    };

    VkMemoryRequirements2 depthImagesMemoryRequirements = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    vkGetImageMemoryRequirements2(device.logical, &depthImagesMemoryRequirementsInfo, &depthImagesMemoryRequirements);

    uint32_t memoryTypeIndex = device.getMemoryTypeIndex(depthImagesMemoryRequirements.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo depthImagesMemoryAllocateInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = nullptr,
        .allocationSize  = depthImagesMemoryRequirements.memoryRequirements.size * swapchainImageCount,
        .memoryTypeIndex = memoryTypeIndex
    };

    vkAllocateMemory(device.logical, &depthImagesMemoryAllocateInfo, nullptr, &depthImagesMemory);

    VkBindImageMemoryInfo* bindDepthImageMemoryInfos = new VkBindImageMemoryInfo[swapchainImageCount];

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        VkBindImageMemoryInfo bindDepthImageMemoryInfo = {
            .sType        = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
            .pNext        = nullptr,
            .image        = depthImages[i],
            .memory       = depthImagesMemory,
            .memoryOffset = i * depthImagesMemoryRequirements.memoryRequirements.size
        };

        bindDepthImageMemoryInfos[i] = bindDepthImageMemoryInfo;
    }

    vkBindImageMemory2(device.logical, swapchainImageCount, bindDepthImageMemoryInfos);

    delete[] bindDepthImageMemoryInfos;

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        // Create the swapchain image views.
        VkImageSubresourceRange subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        };

        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .image            = swapchainImages[i],
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = surfaceFormat.format,
            .components       = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = subresourceRange
        };

        vkCreateImageView(device.logical, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]);

        // Create the depth image views.
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        VkImageViewCreateInfo depthImageViewCreateInfo = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .image            = depthImages[i],
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = createInfo.depthFormat,
            .components       = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = subresourceRange
        };

        vkCreateImageView(device.logical, &depthImageViewCreateInfo, nullptr, &depthImageViews[i]);

        // Create the framebuffers.
        VkImageView attachments[] = {
            swapchainImageViews[i],
            depthImageViews[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext           = nullptr,
            .flags           = 0,
            .renderPass      = createInfo.renderPass,
            .attachmentCount = COUNT_OF(attachments),
            .pAttachments    = attachments,
            .width           = extent.width,
            .height          = extent.height,
            .layers          = 1
        };

        vkCreateFramebuffer(device.logical, &framebufferCreateInfo, nullptr, &framebuffers[i]);
    }

    // Create the command pool.
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = device.queueFamilyIndex
    };

    vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &commandPool);

    // Allocate the command buffers.
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = swapchainImageCount
    };

    vkAllocateCommandBuffers(device.logical, &commandBufferAllocateInfo, commandBuffers);

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        // Create the semaphores.
        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        vkCreateSemaphore(device.logical, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]);
        vkCreateSemaphore(device.logical, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);

        // Create the fences.
        VkFenceCreateInfo fenceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        vkCreateFence(device.logical, &fenceCreateInfo, nullptr, &frameFences[i]);
    }

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        imageFences[i] = frameFences[i % framesInFlight];
    }

    // Get the device queues.
    vkGetDeviceQueue(device.logical, device.queueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device.logical, device.queueFamilyIndex, 1, &presentQueue);
}

void Renderer::destroy(VkDevice device) {
    vkWaitForFences(device, framesInFlight, frameFences, VK_TRUE, UINT64_MAX);

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        vkDestroyFence(device, frameFences[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    vkFreeMemory(device, depthImagesMemory, nullptr);

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
        vkDestroyImageView(device, depthImageViews[i], nullptr);
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
        vkDestroyImage(device, depthImages[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    delete[] frameFences;
    delete[] imageFences;
    delete[] renderFinishedSemaphores;
    delete[] imageAvailableSemaphores;
    delete[] commandBuffers;
    delete[] framebuffers;
    delete[] depthImageViews;
    delete[] swapchainImageViews;
    delete[] depthImages;
    delete[] swapchainImages;
}

void Renderer::recordCommandBuffers(VkRenderPass renderPass, VkExtent2D extent) {
    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .pInheritanceInfo = nullptr
        };

        vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);

        VkRect2D renderArea = {
            .offset = { 0, 0 },
            .extent = extent
        };

        VkClearValue clearValues[] = {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 1.0f, 0 }
        };

        VkRenderPassBeginInfo renderPassBeginInfo = {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext           = nullptr,
            .renderPass      = renderPass,
            .framebuffer     = framebuffers[i],
            .renderArea      = renderArea,
            .clearValueCount = COUNT_OF(clearValues),
            .pClearValues    = clearValues
        };

        VkSubpassBeginInfo subpassBeginInfo = {
            .sType    = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
            .pNext    = nullptr,
            .contents = VK_SUBPASS_CONTENTS_INLINE
        };

        vkCmdBeginRenderPass2(commandBuffers[i], &renderPassBeginInfo, &subpassBeginInfo);

        VkSubpassEndInfo subpassEndInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
            .pNext = nullptr
        };

        vkCmdEndRenderPass2(commandBuffers[i], &subpassEndInfo);

        vkEndCommandBuffer(commandBuffers[i]);
    }
}

bool Renderer::draw(VkDevice device) {
    if (vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex) == VK_ERROR_OUT_OF_DATE_KHR) {
        return false;
    }

    vkWaitForFences(device, 1, &imageFences[imageIndex], VK_TRUE, UINT64_MAX);
    imageFences[imageIndex] = frameFences[frameIndex];

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = nullptr,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &imageAvailableSemaphores[frameIndex],
        .pWaitDstStageMask    = &waitStage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &renderFinishedSemaphores[frameIndex]
    };

    vkWaitForFences(device, 1, &frameFences[frameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &frameFences[frameIndex]);

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameFences[frameIndex]);

    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &renderFinishedSemaphores[frameIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &swapchain,
        .pImageIndices      = &imageIndex,
        .pResults           = nullptr
    };

    vkQueuePresentKHR(presentQueue, &presentInfo);

    frameIndex = (frameIndex + 1) % framesInFlight;

    return true;
}
