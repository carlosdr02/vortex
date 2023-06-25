#include "graphics.h"

#include <string.h>

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

static auto getRequiredDeviceExtensions() {
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

        for (uint32_t i = 0; i < extensionPropertyCount; ++i) {
            if (strcmp(requiredExtension, extensionProperties[i].extensionName) == 0) {
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

    std::array<const char*, 4> deviceExtensions = getRequiredDeviceExtensions();

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &vulkan13Features,
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

static VkSwapchainKHR createSwapchain(VkDevice device, const RendererCreateInfo& createInfo, VkSwapchainKHR oldSwapchain) {
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

    VkSwapchainKHR swapchain;
    vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);

    return swapchain;
}

Renderer::Renderer(Device& device, const RendererCreateInfo& createInfo) : framesInFlight(createInfo.framesInFlight), uniformDataSize(createInfo.uniformDataSize) {
    // Create the swapchain.
    swapchain = createSwapchain(device.logical, createInfo, VK_NULL_HANDLE);

    // Create the command pools.
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = device.renderQueue.familyIndex
    };

    vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &framesCommandPool);

    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &imagesCommandPool);

    // Create the descriptor set layout.
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2];

    descriptorSetLayoutBindings[0].binding            = 0;
    descriptorSetLayoutBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorSetLayoutBindings[0].descriptorCount    = 1;
    descriptorSetLayoutBindings[0].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    descriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;

    descriptorSetLayoutBindings[1].binding            = 1;
    descriptorSetLayoutBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBindings[1].descriptorCount    = 1;
    descriptorSetLayoutBindings[1].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    descriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = nullptr,
        .flags        = 0,
        .bindingCount = COUNT_OF(descriptorSetLayoutBindings),
        .pBindings    = descriptorSetLayoutBindings
    };

    vkCreateDescriptorSetLayout(device.logical, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);

    // Create the descriptor pool.
    VkDescriptorPoolSize descriptorPoolSizes[2];

    descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorPoolSizes[0].descriptorCount = framesInFlight;

    descriptorPoolSizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[1].descriptorCount = framesInFlight;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .maxSets       = framesInFlight,
        .poolSizeCount = COUNT_OF(descriptorPoolSizes),
        .pPoolSizes    = descriptorPoolSizes
    };

    vkCreateDescriptorPool(device.logical, &descriptorPoolCreateInfo, nullptr, &descriptorPool);

    // Allocate the command buffers.
    frameCommandBuffers = new VkCommandBuffer[framesInFlight];
    imageCommandBuffers = new VkCommandBuffer[framesInFlight];

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = framesCommandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = framesInFlight
    };

    vkAllocateCommandBuffers(device.logical, &commandBufferAllocateInfo, frameCommandBuffers);

    commandBufferAllocateInfo.commandPool = imagesCommandPool;

    vkAllocateCommandBuffers(device.logical, &commandBufferAllocateInfo, imageCommandBuffers);

    // Allocate the descriptor sets.
    descriptorSets = new VkDescriptorSet[framesInFlight];

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(framesInFlight, descriptorSetLayout);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = nullptr,
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = framesInFlight,
        .pSetLayouts        = descriptorSetLayouts.data()
    };

    vkAllocateDescriptorSets(device.logical, &descriptorSetAllocateInfo, descriptorSets);

    // Create the uniform buffer.
    uniformBuffer = Buffer(device, framesInFlight * uniformDataSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Map the uniform buffer memory.
    vkMapMemory(device.logical, uniformBuffer.memory, 0, VK_WHOLE_SIZE, 0, &uniformBufferData);

    // Update the descriptor sets.
    VkDescriptorBufferInfo* descriptorBufferInfos = new VkDescriptorBufferInfo[framesInFlight];
    VkWriteDescriptorSet* writeDescriptorSets = new VkWriteDescriptorSet[framesInFlight];

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        descriptorBufferInfos[i].buffer = uniformBuffer;
        descriptorBufferInfos[i].offset = i * uniformDataSize;
        descriptorBufferInfos[i].range  = uniformDataSize;

        writeDescriptorSets[i].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[i].pNext            = nullptr;
        writeDescriptorSets[i].dstSet           = descriptorSets[i];
        writeDescriptorSets[i].dstBinding       = 1;
        writeDescriptorSets[i].dstArrayElement  = 0;
        writeDescriptorSets[i].descriptorCount  = 1;
        writeDescriptorSets[i].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[i].pImageInfo       = nullptr;
        writeDescriptorSets[i].pBufferInfo      = &descriptorBufferInfos[i];
        writeDescriptorSets[i].pTexelBufferView = nullptr;
    }

    vkUpdateDescriptorSets(device.logical, framesInFlight, writeDescriptorSets, 0, nullptr);

    delete[] writeDescriptorSets;
    delete[] descriptorBufferInfos;

    // Create the swapchain resources.
    offscreenImages = new VkImage[framesInFlight];
    offscreenImageViews = new VkImageView[framesInFlight];

    createSwapchainResources(device, createInfo);

    // Create the fences and semaphores.
    frameFences = new VkFence[framesInFlight];
    imageAvailableSemaphores = new VkSemaphore[framesInFlight];
    renderFinishedSemaphores = new VkSemaphore[framesInFlight];

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        VkFenceCreateInfo fenceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        vkCreateFence(device.logical, &fenceCreateInfo, nullptr, &frameFences[i]);

        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        vkCreateSemaphore(device.logical, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]);
        vkCreateSemaphore(device.logical, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);
    }
}

void Renderer::recreate(Device& device, const RendererCreateInfo& createInfo) {
    // Destroy the old swapchain resources.
    destroySwapchainResources(device.logical);

    // Store the old swapchain.
    VkSwapchainKHR oldSwapchain = swapchain;

    // Create the new swapchain.
    swapchain = createSwapchain(device.logical, createInfo, oldSwapchain);

    // Destroy the old swapchain.
    vkDestroySwapchainKHR(device.logical, oldSwapchain, nullptr);

    // Create the new swapchain resources.
    createSwapchainResources(device, createInfo);
}

void Renderer::destroy(VkDevice device) {
    for (uint32_t i = 0; i < framesInFlight; ++i) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, frameFences[i], nullptr);
    }

    destroySwapchainResources(device);

    uniformBuffer.destroy(device);

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyCommandPool(device, imagesCommandPool, nullptr);
    vkDestroyCommandPool(device, framesCommandPool, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);

    delete[] renderFinishedSemaphores;
    delete[] imageAvailableSemaphores;
    delete[] frameFences;
    delete[] offscreenImageViews;
    delete[] offscreenImages;
    delete[] descriptorSets;
    delete[] imageCommandBuffers;
    delete[] frameCommandBuffers;
}

void Renderer::recordCommandBuffers(VkDevice device) {
    vkResetCommandPool(device, framesCommandPool, 0);

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .pInheritanceInfo = nullptr
        };

        vkBeginCommandBuffer(frameCommandBuffers[i], &commandBufferBeginInfo);

        VkImageMemoryBarrier2 imageMemoryBarrier = {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext               = nullptr,
            .srcStageMask        = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask       = VK_ACCESS_2_NONE,
            .dstStageMask        = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
            .dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = offscreenImages[i],
            .subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };

        VkDependencyInfo dependencyInfo = {
            .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext                    = nullptr,
            .dependencyFlags          = 0,
            .memoryBarrierCount       = 0,
            .pMemoryBarriers          = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers    = nullptr,
            .imageMemoryBarrierCount  = 1,
            .pImageMemoryBarriers     = &imageMemoryBarrier
        };

        vkCmdPipelineBarrier2(frameCommandBuffers[i], &dependencyInfo);

        // TODO: Trace rays.

        imageMemoryBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
        imageMemoryBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        imageMemoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        vkCmdPipelineBarrier2(frameCommandBuffers[i], &dependencyInfo);

        vkEndCommandBuffer(frameCommandBuffers[i]);
    }
}

bool Renderer::render(Device& device, const void* uniformData, VkExtent2D extent) {
    uint32_t imageIndex;

    if (vkAcquireNextImageKHR(device.logical, swapchain, UINT64_MAX, imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex) == VK_ERROR_OUT_OF_DATE_KHR) {
        return false;
    }

    vkWaitForFences(device.logical, 1, &frameFences[frameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(device.logical, 1, &frameFences[frameIndex]);

    VkDeviceSize offset = frameIndex * uniformDataSize;
    memcpy(static_cast<char*>(uniformBufferData) + offset, uniformData, uniformDataSize);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(imageCommandBuffers[frameIndex], &commandBufferBeginInfo);

    VkImageMemoryBarrier2 imageMemoryBarrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext               = nullptr,
        .srcStageMask        = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .srcAccessMask       = VK_ACCESS_2_NONE,
        .dstStageMask        = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = swapchainImages[imageIndex],
        .subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };

    VkDependencyInfo dependencyInfo = {
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext                    = nullptr,
        .dependencyFlags          = 0,
        .memoryBarrierCount       = 0,
        .pMemoryBarriers          = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers    = nullptr,
        .imageMemoryBarrierCount  = 1,
        .pImageMemoryBarriers     = &imageMemoryBarrier
    };

    vkCmdPipelineBarrier2(imageCommandBuffers[frameIndex], &dependencyInfo);

    VkImageSubresourceLayers imageSubresourceLayers = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = 1
    };

    VkImageBlit2 imageBlit = {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .pNext          = nullptr,
        .srcSubresource = imageSubresourceLayers,
        .srcOffsets     = { { 0, 0, 0 }, { static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1 } },
        .dstSubresource = imageSubresourceLayers,
        .dstOffsets     = { { 0, static_cast<int32_t>(extent.height), 0 }, { static_cast<int32_t>(extent.width), 0, 1 } }
    };

    VkBlitImageInfo2 blitImageInfo = {
        .sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .pNext          = nullptr,
        .srcImage       = offscreenImages[frameIndex],
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage       = swapchainImages[imageIndex],
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount    = 1,
        .pRegions       = &imageBlit,
        .filter         = VK_FILTER_NEAREST
    };

    vkCmdBlitImage2(imageCommandBuffers[frameIndex], &blitImageInfo);

    imageMemoryBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_NONE;
    imageMemoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    vkCmdPipelineBarrier2(imageCommandBuffers[frameIndex], &dependencyInfo);

    vkEndCommandBuffer(imageCommandBuffers[frameIndex]);

    VkSemaphoreSubmitInfo semaphoreSubmitInfos[2];

    semaphoreSubmitInfos[0].sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    semaphoreSubmitInfos[0].pNext       = nullptr;
    semaphoreSubmitInfos[0].semaphore   = imageAvailableSemaphores[frameIndex];
    semaphoreSubmitInfos[0].value       = 0;
    semaphoreSubmitInfos[0].stageMask   = VK_PIPELINE_STAGE_2_BLIT_BIT;
    semaphoreSubmitInfos[0].deviceIndex = 0;

    semaphoreSubmitInfos[1].sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    semaphoreSubmitInfos[1].pNext       = nullptr;
    semaphoreSubmitInfos[1].semaphore   = renderFinishedSemaphores[frameIndex];
    semaphoreSubmitInfos[1].value       = 0;
    semaphoreSubmitInfos[1].stageMask   = VK_PIPELINE_STAGE_2_BLIT_BIT;
    semaphoreSubmitInfos[1].deviceIndex = 0;

    VkCommandBufferSubmitInfo commandBufferSubmitInfos[2];

    commandBufferSubmitInfos[0].sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferSubmitInfos[0].pNext         = nullptr;
    commandBufferSubmitInfos[0].commandBuffer = frameCommandBuffers[frameIndex];
    commandBufferSubmitInfos[0].deviceMask    = 0;

    commandBufferSubmitInfos[1].sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferSubmitInfos[1].pNext         = nullptr;
    commandBufferSubmitInfos[1].commandBuffer = imageCommandBuffers[frameIndex];
    commandBufferSubmitInfos[1].deviceMask    = 0;

    VkSubmitInfo2 submitInfos[2];

    submitInfos[0].sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfos[0].pNext                    = nullptr;
    submitInfos[0].flags                    = 0;
    submitInfos[0].waitSemaphoreInfoCount   = 0;
    submitInfos[0].pWaitSemaphoreInfos      = nullptr;
    submitInfos[0].commandBufferInfoCount   = 1;
    submitInfos[0].pCommandBufferInfos      = &commandBufferSubmitInfos[0];
    submitInfos[0].signalSemaphoreInfoCount = 0;
    submitInfos[0].pSignalSemaphoreInfos    = nullptr;

    submitInfos[1].sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfos[1].pNext                    = nullptr;
    submitInfos[1].flags                    = 0;
    submitInfos[1].waitSemaphoreInfoCount   = 1;
    submitInfos[1].pWaitSemaphoreInfos      = &semaphoreSubmitInfos[0];
    submitInfos[1].commandBufferInfoCount   = 1;
    submitInfos[1].pCommandBufferInfos      = &commandBufferSubmitInfos[1];
    submitInfos[1].signalSemaphoreInfoCount = 1;
    submitInfos[1].pSignalSemaphoreInfos    = &semaphoreSubmitInfos[1];

    vkQueueSubmit2(device.renderQueue, COUNT_OF(submitInfos), submitInfos, frameFences[frameIndex]);

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

    vkQueuePresentKHR(device.renderQueue, &presentInfo);

    frameIndex = (frameIndex + 1) % framesInFlight;

    return true;
}

void Renderer::waitIdle(VkDevice device) {
    vkWaitForFences(device, framesInFlight, frameFences, VK_TRUE, UINT64_MAX);
}

void Renderer::createSwapchainResources(Device& device, const RendererCreateInfo& createInfo) {
    // Get the swapchain images.
    vkGetSwapchainImagesKHR(device.logical, swapchain, &swapchainImageCount, nullptr);

    swapchainImages = new VkImage[swapchainImageCount];
    vkGetSwapchainImagesKHR(device.logical, swapchain, &swapchainImageCount, swapchainImages);

    // Create the off-screen images.
    for (uint32_t i = 0; i < framesInFlight; ++i) {
        VkExtent2D extent = createInfo.surfaceCapabilities->currentExtent;

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

        vkCreateImage(device.logical, &imageCreateInfo, nullptr, &offscreenImages[i]);
    }

    // Allocate the off-screen images memory.
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device.logical, offscreenImages[0], &memoryRequirements);

    uint32_t memoryTypeIndex = device.getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = nullptr,
        .allocationSize  = framesInFlight * memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    vkAllocateMemory(device.logical, &memoryAllocateInfo, nullptr, &offscreenImagesMemory);

    // Bind the off-screen images memory.
    VkBindImageMemoryInfo* bindImageMemoryInfos = new VkBindImageMemoryInfo[framesInFlight];

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        bindImageMemoryInfos[i].sType        = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
        bindImageMemoryInfos[i].pNext        = nullptr;
        bindImageMemoryInfos[i].image        = offscreenImages[i];
        bindImageMemoryInfos[i].memory       = offscreenImagesMemory;
        bindImageMemoryInfos[i].memoryOffset = i * memoryRequirements.size;
    }

    vkBindImageMemory2(device.logical, framesInFlight, bindImageMemoryInfos);

    delete[] bindImageMemoryInfos;

    // Create the off-screen image views.
    for (uint32_t i = 0; i < framesInFlight; ++i) {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .image            = offscreenImages[i],
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = VK_FORMAT_R16G16B16A16_SFLOAT,
            .components       = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };

        vkCreateImageView(device.logical, &imageViewCreateInfo, nullptr, &offscreenImageViews[i]);
    }

    // Update the descriptor sets.
    VkDescriptorImageInfo* descriptorImageInfos = new VkDescriptorImageInfo[framesInFlight];
    VkWriteDescriptorSet* writeDescriptorSets = new VkWriteDescriptorSet[framesInFlight];

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        descriptorImageInfos[i].sampler     = VK_NULL_HANDLE;
        descriptorImageInfos[i].imageView   = offscreenImageViews[i];
        descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        writeDescriptorSets[i].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[i].pNext            = nullptr;
        writeDescriptorSets[i].dstSet           = descriptorSets[i];
        writeDescriptorSets[i].dstBinding       = 0;
        writeDescriptorSets[i].dstArrayElement  = 0;
        writeDescriptorSets[i].descriptorCount  = 1;
        writeDescriptorSets[i].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[i].pImageInfo       = &descriptorImageInfos[i];
        writeDescriptorSets[i].pBufferInfo      = nullptr;
        writeDescriptorSets[i].pTexelBufferView = nullptr;
    }

    vkUpdateDescriptorSets(device.logical, framesInFlight, writeDescriptorSets, 0, nullptr);

    delete[] writeDescriptorSets;
    delete[] descriptorImageInfos;
}

void Renderer::destroySwapchainResources(VkDevice device) {
    for (uint32_t i = 0; i < framesInFlight; ++i) {
        vkDestroyImageView(device, offscreenImageViews[i], nullptr);
    }

    vkFreeMemory(device, offscreenImagesMemory, nullptr);

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        vkDestroyImage(device, offscreenImages[i], nullptr);
    }

    delete[] swapchainImages;
}
