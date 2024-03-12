#include "graphics.h"

#include <string.h>

#include <imgui_impl_vulkan.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

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

static bool isDiscrete(VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

static bool supportsRayTracing(VkPhysicalDevice physicalDevice) {
    uint32_t extensionPropertyCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);

    VkExtensionProperties* extensionProperties = new VkExtensionProperties[extensionPropertyCount];
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, extensionProperties);

    bool supportsRayTracing = false;

    for (uint32_t i = 0; i < extensionPropertyCount; ++i) {
        if (strcmp(extensionProperties[i].extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0) {
            supportsRayTracing = true;
            break;
        }
    }

    delete[] extensionProperties;

    return supportsRayTracing;
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
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);

    physical = physicalDevices[0]; // TODO

    delete[] physicalDevices;

    // Select a queue family.
    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyPropertyCount, nullptr);

    VkQueueFamilyProperties* queueFamilyProperties = new VkQueueFamilyProperties[queueFamilyPropertyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyPropertyCount, queueFamilyProperties);

    for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i) {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
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

    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &vulkan13Features,
        .flags                   = 0,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = ARRAY_SIZE(deviceExtensions),
        .ppEnabledExtensionNames = deviceExtensions,
        .pEnabledFeatures        = nullptr
    };

    vkCreateDevice(physical, &deviceCreateInfo, nullptr, &logical);

    // Get the device queue.
    vkGetDeviceQueue(logical, renderQueue.familyIndex, 0, &renderQueue);
}

void Device::destroy() {
    vkDestroyDevice(logical, nullptr);
}

static uint32_t clamp(int d, uint32_t min, uint32_t max) {
    const uint32_t t = d < min ? min : d;
    return t > max ? max : t;
}

VkSurfaceCapabilitiesKHR Device::getSurfaceCapabilities(VkSurfaceKHR surface, GLFWwindow* window) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &surfaceCapabilities);

    uint32_t maxImageCount = surfaceCapabilities.maxImageCount != 0 ? surfaceCapabilities.maxImageCount : UINT32_MAX;
    surfaceCapabilities.minImageCount = clamp(3, surfaceCapabilities.minImageCount, maxImageCount);

    VkExtent2D& currentExtent = surfaceCapabilities.currentExtent;

    if (currentExtent.width == UINT32_MAX) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D minExtent = surfaceCapabilities.minImageExtent;
        VkExtent2D maxExtent = surfaceCapabilities.maxImageExtent;

        currentExtent = {
            .width  = clamp(width, minExtent.width, maxExtent.width),
            .height = clamp(height, minExtent.height, maxExtent.height)
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
            if (surfaceFormats[i].format == format && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
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

VkRenderPass createRenderPass(VkDevice device, VkFormat format) {
    VkAttachmentDescription2 attachmentDescription = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .pNext          = nullptr,
        .flags          = 0,
        .format         = format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference2 attachmentReference = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .pNext      = nullptr,
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .aspectMask = VK_IMAGE_ASPECT_NONE
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
        .pColorAttachments       = &attachmentReference,
        .pResolveAttachments     = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments    = nullptr
    };

    VkMemoryBarrier2 memoryBarriers[2];

    memoryBarriers[0].sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memoryBarriers[0].pNext         = nullptr;
    memoryBarriers[0].srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    memoryBarriers[0].srcAccessMask = VK_ACCESS_2_NONE;
    memoryBarriers[0].dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    memoryBarriers[0].dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    memoryBarriers[1].sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memoryBarriers[1].pNext         = nullptr;
    memoryBarriers[1].srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    memoryBarriers[1].srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    memoryBarriers[1].dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    memoryBarriers[1].dstAccessMask = VK_ACCESS_2_NONE;

    VkSubpassDependency2 subpassDependencies[2];

    subpassDependencies[0].sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
    subpassDependencies[0].pNext           = &memoryBarriers[0];
    subpassDependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].dstSubpass      = 0;
    subpassDependencies[0].dependencyFlags = 0;
    subpassDependencies[0].viewOffset      = 0;

    subpassDependencies[1].sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
    subpassDependencies[1].pNext           = &memoryBarriers[1];
    subpassDependencies[1].srcSubpass      = 0;
    subpassDependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dependencyFlags = 0;
    subpassDependencies[1].viewOffset      = 0;

    VkRenderPassCreateInfo2 renderPassCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .pNext                   = nullptr,
        .flags                   = 0,
        .attachmentCount         = 1,
        .pAttachments            = &attachmentDescription,
        .subpassCount            = 1,
        .pSubpasses              = &subpassDescription,
        .dependencyCount         = ARRAY_SIZE(subpassDependencies),
        .pDependencies           = subpassDependencies,
        .correlatedViewMaskCount = 0,
        .pCorrelatedViewMasks    = nullptr
    };

    VkRenderPass renderPass;
    vkCreateRenderPass2(device, &renderPassCreateInfo, nullptr, &renderPass);

    return renderPass;
}

VkDescriptorPool createGuiDescriptorPool(VkDevice device) {
    VkDescriptorPoolSize descriptorPoolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 1,
        .poolSizeCount = ARRAY_SIZE(descriptorPoolSizes),
        .pPoolSizes    = descriptorPoolSizes
    };

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);

    return descriptorPool;
}

Renderer::Renderer(Device& device, const RendererCreateInfo& createInfo)
    : framesInFlight(createInfo.framesInFlight)
    , frameIndex(0)
{
    createSwapchain(device.logical, createInfo, VK_NULL_HANDLE);

    // Create the command pool.
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = device.renderQueue.familyIndex
    };

    vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &commandPool);

    // Get the swapchain image count.
    vkGetSwapchainImagesKHR(device.logical, swapchain, &swapchainImageCount, nullptr);

    allocateSwapchainResourcesHostMemory();
    createSwapchainResources(device.logical, createInfo);
    createFrameResources(device.logical);
}

void Renderer::destroy(VkDevice device) {
    destroyFrameResources(device);
    destroySwapchainResources(device);
    freeSwapchainResourcesHostMemory();

    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

bool Renderer::render(Device& device, VkRenderPass renderPass, VkExtent2D extent) {
    vkWaitForFences(device.logical, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;

    if (vkAcquireNextImageKHR(device.logical, swapchain, UINT64_MAX, imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex) == VK_ERROR_OUT_OF_DATE_KHR) {
        return false;
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(commandBuffers[frameIndex], &commandBufferBeginInfo);

    VkClearValue clearValue = {
        0.0f, 0.0f, 0.0f, 1.0f
    };

    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = nullptr,
        .renderPass      = renderPass,
        .framebuffer     = framebuffers[imageIndex],
        .renderArea      = { { 0, 0 }, extent },
        .clearValueCount = 1,
        .pClearValues    = &clearValue
    };

    vkCmdBeginRenderPass(commandBuffers[frameIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffers[frameIndex]);

    vkCmdEndRenderPass(commandBuffers[frameIndex]);

    vkEndCommandBuffer(commandBuffers[frameIndex]);

    vkResetFences(device.logical, 1, &fences[frameIndex]);

    VkSemaphoreSubmitInfo semaphoreSubmitInfos[2];

    semaphoreSubmitInfos[0].sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    semaphoreSubmitInfos[0].pNext       = nullptr;
    semaphoreSubmitInfos[0].semaphore   = imageAvailableSemaphores[frameIndex];
    semaphoreSubmitInfos[0].value       = 0;
    semaphoreSubmitInfos[0].stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    semaphoreSubmitInfos[0].deviceIndex = 0;

    semaphoreSubmitInfos[1].sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    semaphoreSubmitInfos[1].pNext       = nullptr;
    semaphoreSubmitInfos[1].semaphore   = renderFinishedSemaphores[frameIndex];
    semaphoreSubmitInfos[1].value       = 0;
    semaphoreSubmitInfos[1].stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    semaphoreSubmitInfos[1].deviceIndex = 0;

    VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = commandBuffers[frameIndex],
        .deviceMask    = 0
    };

    VkSubmitInfo2 submitInfo = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0,
        .waitSemaphoreInfoCount   = 1,
        .pWaitSemaphoreInfos      = &semaphoreSubmitInfos[0],
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &semaphoreSubmitInfos[1]
    };

    vkQueueSubmit2(device.renderQueue, 1, &submitInfo, fences[frameIndex]);

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
    vkWaitForFences(device, framesInFlight, fences, VK_TRUE, UINT64_MAX);
}

void Renderer::resize(VkDevice device, const RendererCreateInfo& createInfo) {
    destroySwapchainResources(device);

    // Store the old swapchain.
    VkSwapchainKHR oldSwapchain = swapchain;

    // Create the new swapchain.
    createSwapchain(device, createInfo, oldSwapchain);

    // Destroy the old swapchain.
    vkDestroySwapchainKHR(device, oldSwapchain, nullptr);

    // Reallocate the host memory only if the number of swapchain images has changed.
    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);

    if (swapchainImageCount != this->swapchainImageCount) {
        freeSwapchainResourcesHostMemory();

        this->swapchainImageCount = swapchainImageCount;

        allocateSwapchainResourcesHostMemory();
    }

    createSwapchainResources(device, createInfo);
}

void Renderer::setFramesInFlight(VkDevice device, uint32_t framesInFlight) {
    destroyFrameResources(device);

    this->framesInFlight = framesInFlight;
    frameIndex = 0;

    createFrameResources(device);
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
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = surfaceCapabilities->currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = VK_PRESENT_MODE_MAILBOX_KHR, // TODO
        .clipped               = VK_TRUE,
        .oldSwapchain          = oldSwapchain
    };

    vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
}

void Renderer::allocateSwapchainResourcesHostMemory() {
    swapchainImages = new VkImage[swapchainImageCount];
    swapchainImageViews = new VkImageView[swapchainImageCount];
    framebuffers = new VkFramebuffer[swapchainImageCount];
}

void Renderer::createSwapchainResources(VkDevice device, const RendererCreateInfo& createInfo) {
    // Get the swapchain images.
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        // Create the swapchain image views.
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .image            = swapchainImages[i],
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = createInfo.surfaceFormat.format,
            .components       = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };

        vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]);

        // Create the framebuffers.
        VkExtent2D extent = createInfo.surfaceCapabilities->currentExtent;

        VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext           = nullptr,
            .flags           = 0,
            .renderPass      = createInfo.renderPass,
            .attachmentCount = 1,
            .pAttachments    = &swapchainImageViews[i],
            .width           = extent.width,
            .height          = extent.height,
            .layers          = 1
        };

        vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffers[i]);
    }
}

void Renderer::createFrameResources(VkDevice device) {
    // Allocate the command buffers.
    commandBuffers = new VkCommandBuffer[framesInFlight];

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = framesInFlight
    };

    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers);

    // Create the semaphores and fences.
    imageAvailableSemaphores = new VkSemaphore[framesInFlight];
    renderFinishedSemaphores = new VkSemaphore[framesInFlight];
    fences = new VkFence[framesInFlight];

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);

        VkFenceCreateInfo fenceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        vkCreateFence(device, &fenceCreateInfo, nullptr, &fences[i]);
    }
}

void Renderer::freeSwapchainResourcesHostMemory() {
    delete[] framebuffers;
    delete[] swapchainImageViews;
    delete[] swapchainImages;
}

void Renderer::destroySwapchainResources(VkDevice device) {
    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
}

void Renderer::destroyFrameResources(VkDevice device) {
    for (uint32_t i = 0; i < framesInFlight; ++i) {
        vkDestroyFence(device, fences[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    }

    delete[] fences;
    delete[] renderFinishedSemaphores;
    delete[] imageAvailableSemaphores;

    vkFreeCommandBuffers(device, commandPool, framesInFlight, commandBuffers);

    delete[] commandBuffers;
}
