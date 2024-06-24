#include "graphics.h"

#include <string.h>

#include <fstream>

#include <imgui_impl_vulkan.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

static PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelines;
static PFN_vkCmdTraceRaysKHR vkCmdTraceRays;

VkInstance createInstance() {
    VkApplicationInfo applicationInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = "Vortex",
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

    // Get the ray tracing pipeline properties.
    rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    rtProperties.pNext = nullptr;

    VkPhysicalDeviceProperties2 physicalDeviceProperties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rtProperties
    };

    vkGetPhysicalDeviceProperties2(physical, &physicalDeviceProperties);

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
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM
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

void loadFunctionPointers(VkDevice device) {
    vkCreateRayTracingPipelines = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
    vkCmdTraceRays = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
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

    // Allocate the device memory.
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

VkRenderPass createRenderPass(VkDevice device, VkFormat format, bool clear) {
    VkAttachmentDescription2 attachmentDescription = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .pNext          = nullptr,
        .flags          = 0,
        .format         = format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
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

    if (clear) {
        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        memoryBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        memoryBarriers[0].srcAccessMask = VK_ACCESS_2_NONE;
    }
    else {
        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        memoryBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
        memoryBarriers[0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }

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

VkPipelineLayout createPipelineLayout(VkDevice device, uint32_t setLayoutCount, const VkDescriptorSetLayout* setLayouts) {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = setLayoutCount,
        .pSetLayouts            = setLayouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr
    };

    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

    return pipelineLayout;
}

static VkShaderModule createShaderModule(VkDevice device, const char* fileName) {
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    size_t codeSize = file.tellg();
    char* code = new char[codeSize];

    file.seekg(0);
    file.read(code, codeSize);
    file.close();

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .codeSize = codeSize,
        .pCode    = (uint32_t*)code
    };

    VkShaderModule shaderModule;
    vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);

    delete[] code;

    return shaderModule;
}

static void populateShaderStageCreateInfo(VkPipelineShaderStageCreateInfo& createInfo, VkShaderStageFlagBits stage, VkShaderModule module) {
    createInfo.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.pNext               = nullptr;
    createInfo.flags               = 0;
    createInfo.stage               = stage;
    createInfo.module              = module;
    createInfo.pName               = "main";
    createInfo.pSpecializationInfo = nullptr;
}

VkPipeline createRayTracingPipeline(VkDevice device, uint32_t entryCount, const ShaderBindingTableEntry* entries, VkPipelineLayout pipelineLayout) {
    uint32_t shaderCount = 0;

    for (uint32_t i = 0; i < entryCount; ++i) {
        if (entries[i].stage != SHADER_BINDING_TABLE_STAGE_HIT) {
            ++shaderCount;
        }
        else {
            if (entries[i].closestHitShader != nullptr) {
                ++shaderCount;
            }

            if (entries[i].anyHitShader != nullptr) {
                ++shaderCount;
            }

            if (entries[i].intersectionShader != nullptr) {
                ++shaderCount;
            }
        }
    }

    VkShaderModule* shaderModules = new VkShaderModule[shaderCount];
    VkPipelineShaderStageCreateInfo* shaderStageCreateInfos = new VkPipelineShaderStageCreateInfo[shaderCount];
    VkRayTracingShaderGroupCreateInfoKHR* shaderGroupCreateInfos = new VkRayTracingShaderGroupCreateInfoKHR[entryCount];

    for (uint32_t i = 0, j = 0; i < entryCount; ++i) {
        shaderGroupCreateInfos[i].sType                           = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupCreateInfos[i].pNext                           = nullptr;
        shaderGroupCreateInfos[i].generalShader                   = VK_SHADER_UNUSED_KHR;
        shaderGroupCreateInfos[i].closestHitShader                = VK_SHADER_UNUSED_KHR;
        shaderGroupCreateInfos[i].anyHitShader                    = VK_SHADER_UNUSED_KHR;
        shaderGroupCreateInfos[i].intersectionShader              = VK_SHADER_UNUSED_KHR;
        shaderGroupCreateInfos[i].pShaderGroupCaptureReplayHandle = nullptr;

        if (entries[i].stage != SHADER_BINDING_TABLE_STAGE_HIT) {
            shaderModules[j] = createShaderModule(device, entries[i].generalShader);

            if (entries[i].stage == SHADER_BINDING_TABLE_STAGE_RAYGEN) {
                populateShaderStageCreateInfo(shaderStageCreateInfos[j], VK_SHADER_STAGE_RAYGEN_BIT_KHR, shaderModules[j]);
            }
            else {
                populateShaderStageCreateInfo(shaderStageCreateInfos[j], VK_SHADER_STAGE_MISS_BIT_KHR, shaderModules[j]);
            }

            shaderGroupCreateInfos[i].type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroupCreateInfos[i].generalShader = j++;
        }
        else {
            if (entries[i].closestHitShader != nullptr) {
                shaderModules[j] = createShaderModule(device, entries[i].closestHitShader);
                populateShaderStageCreateInfo(shaderStageCreateInfos[j], VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, shaderModules[j]);
                shaderGroupCreateInfos[i].closestHitShader = j++;
            }

            if (entries[i].anyHitShader != nullptr) {
                shaderModules[j] = createShaderModule(device, entries[i].anyHitShader);
                populateShaderStageCreateInfo(shaderStageCreateInfos[j], VK_SHADER_STAGE_ANY_HIT_BIT_KHR, shaderModules[j]);
                shaderGroupCreateInfos[i].anyHitShader = j++;
            }

            if (entries[i].intersectionShader != nullptr) {
                shaderModules[j] = createShaderModule(device, entries[i].intersectionShader);
                populateShaderStageCreateInfo(shaderStageCreateInfos[j], VK_SHADER_STAGE_INTERSECTION_BIT_KHR, shaderModules[j]);
                shaderGroupCreateInfos[i].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
                shaderGroupCreateInfos[i].intersectionShader = j++;
            }
            else {
                shaderGroupCreateInfos[i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            }
        }
    }

    VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = {
        .sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .pNext                        = nullptr,
        .flags                        = 0,
        .stageCount                   = shaderCount,
        .pStages                      = shaderStageCreateInfos,
        .groupCount                   = entryCount,
        .pGroups                      = shaderGroupCreateInfos,
        .maxPipelineRayRecursionDepth = 0,
        .pLibraryInfo                 = nullptr,
        .pLibraryInterface            = nullptr,
        .pDynamicState                = nullptr,
        .layout                       = pipelineLayout,
        .basePipelineHandle           = VK_NULL_HANDLE,
        .basePipelineIndex            = -1
    };

    VkPipeline pipeline;
    vkCreateRayTracingPipelines(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCreateInfo, nullptr, &pipeline);

    for (uint32_t i = 0; i < shaderCount; ++i) {
        vkDestroyShaderModule(device, shaderModules[i], nullptr);
    }

    delete[] shaderGroupCreateInfos;
    delete[] shaderStageCreateInfos;
    delete[] shaderModules;

    return pipeline;
}

static uint32_t alignNumber(uint32_t number, uint32_t alignment) {
    return (number + alignment - 1) & ~(alignment - 1);
}

Renderer::Renderer(Device& device, const RendererCreateInfo& createInfo) : framesInFlight(createInfo.framesInFlight) {
    createSwapchain(device.logical, createInfo, VK_NULL_HANDLE);

    // Create the command pools.
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = device.renderQueue.familyIndex
    };

    vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &normalCommandPool);

    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &transientCommandPool);

    // Create the descriptor set layout.
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {
        .binding            = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = nullptr,
        .flags        = 0,
        .bindingCount = 1,
        .pBindings    = &descriptorSetLayoutBinding
    };

    vkCreateDescriptorSetLayout(device.logical, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);

    // Get the swapchain image count.
    vkGetSwapchainImagesKHR(device.logical, swapchain, &swapchainImageCount, nullptr);

    allocateSwapchainResourcesMemory();
    createSwapchainResources(device.logical, createInfo);
    createFrameResources(device.logical);
    allocateOffscreenResourcesMemory();
    createOffscreenResources(device, createInfo);
}

void Renderer::destroy(VkDevice device) {
    destroyOffscreenResources(device);
    freeOffscreenResourcesMemory();
    destroyFrameResources(device);
    destroySwapchainResources(device);
    freeSwapchainResourcesMemory();

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyCommandPool(device, transientCommandPool, nullptr);
    vkDestroyCommandPool(device, normalCommandPool, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Renderer::recordCommandBuffers(VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline rayTracingPipeline) {
    vkResetCommandPool(device, normalCommandPool, 0);

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .pInheritanceInfo = nullptr
        };

        vkBeginCommandBuffer(normalCommandBuffers[i], &commandBufferBeginInfo);

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

        vkCmdPipelineBarrier2(normalCommandBuffers[i], &dependencyInfo);

        vkCmdBindDescriptorSets(normalCommandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
        vkCmdBindPipeline(normalCommandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline);

        imageMemoryBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
        imageMemoryBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        imageMemoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        vkCmdPipelineBarrier2(normalCommandBuffers[i], &dependencyInfo);

        vkEndCommandBuffer(normalCommandBuffers[i]);
    }
}

bool Renderer::render(Device& device, VkRenderPass renderPass, VkExtent2D extent) {
    vkWaitForFences(device.logical, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;

    if (vkAcquireNextImageKHR(device.logical, swapchain, UINT64_MAX, imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex) == VK_ERROR_OUT_OF_DATE_KHR) {
        return false;
    }

    vkResetFences(device.logical, 1, &fences[frameIndex]);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(transientCommandBuffers[frameIndex], &commandBufferBeginInfo);

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

    vkCmdPipelineBarrier2(transientCommandBuffers[frameIndex], &dependencyInfo);

    VkImageBlit2 imageBlit = {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .pNext          = nullptr,
        .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .srcOffsets     = { { 0, 0, 0 }, { (int32_t)extent.width, (int32_t)extent.height, 1 } },
        .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .dstOffsets     = { { 0, (int32_t)extent.height, 0 }, { (int32_t)extent.width, 0, 1 } }
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

    vkCmdBlitImage2(transientCommandBuffers[frameIndex], &blitImageInfo);

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

    vkCmdBeginRenderPass(transientCommandBuffers[frameIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(drawData, transientCommandBuffers[frameIndex]);

    vkCmdEndRenderPass(transientCommandBuffers[frameIndex]);

    vkEndCommandBuffer(transientCommandBuffers[frameIndex]);

    VkSemaphoreSubmitInfo waitSemaphoreInfo = {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = imageAvailableSemaphores[frameIndex],
        .value       = 0,
        .stageMask   = VK_PIPELINE_STAGE_2_BLIT_BIT,
        .deviceIndex = 0
    };

    VkSemaphoreSubmitInfo signalSemaphoreInfo = {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = renderFinishedSemaphores[frameIndex],
        .value       = 0,
        .stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .deviceIndex = 0
    };

    VkCommandBufferSubmitInfo normalCommandBufferInfo = {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = normalCommandBuffers[frameIndex],
        .deviceMask    = 0
    };

    VkCommandBufferSubmitInfo transientCommandBufferInfo = {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = transientCommandBuffers[frameIndex],
        .deviceMask    = 0
    };

    VkSubmitInfo2 submitInfos[2];

    submitInfos[0].sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfos[0].pNext                    = nullptr;
    submitInfos[0].flags                    = 0;
    submitInfos[0].waitSemaphoreInfoCount   = 0;
    submitInfos[0].pWaitSemaphoreInfos      = nullptr;
    submitInfos[0].commandBufferInfoCount   = 1;
    submitInfos[0].pCommandBufferInfos      = &normalCommandBufferInfo;
    submitInfos[0].signalSemaphoreInfoCount = 0;
    submitInfos[0].pSignalSemaphoreInfos    = nullptr;

    submitInfos[1].sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfos[1].pNext                    = nullptr;
    submitInfos[1].flags                    = 0;
    submitInfos[1].waitSemaphoreInfoCount   = 1;
    submitInfos[1].pWaitSemaphoreInfos      = &waitSemaphoreInfo;
    submitInfos[1].commandBufferInfoCount   = 1;
    submitInfos[1].pCommandBufferInfos      = &transientCommandBufferInfo;
    submitInfos[1].signalSemaphoreInfoCount = 1;
    submitInfos[1].pSignalSemaphoreInfos    = &signalSemaphoreInfo;

    vkQueueSubmit2(device.renderQueue, ARRAY_SIZE(submitInfos), submitInfos, fences[frameIndex]);

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

void Renderer::resize(Device& device, const RendererCreateInfo& createInfo) {
    destroyOffscreenResources(device.logical);
    destroySwapchainResources(device.logical);

    // Store the old swapchain.
    VkSwapchainKHR oldSwapchain = swapchain;

    // Create the new swapchain.
    createSwapchain(device.logical, createInfo, oldSwapchain);

    // Destroy the old swapchain.
    vkDestroySwapchainKHR(device.logical, oldSwapchain, nullptr);

    // Reallocate the host memory only if the number of swapchain images has changed.
    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(device.logical, swapchain, &swapchainImageCount, nullptr);

    if (swapchainImageCount != this->swapchainImageCount) {
        freeSwapchainResourcesMemory();

        this->swapchainImageCount = swapchainImageCount;

        allocateSwapchainResourcesMemory();
    }

    createSwapchainResources(device.logical, createInfo);
    createOffscreenResources(device, createInfo);
}

void Renderer::setFramesInFlight(Device& device, const RendererCreateInfo& createInfo) {
    destroyOffscreenResources(device.logical);
    freeOffscreenResourcesMemory();
    destroyFrameResources(device.logical);

    framesInFlight = createInfo.framesInFlight;
    frameIndex = 0;

    createFrameResources(device.logical);
    allocateOffscreenResourcesMemory();
    createOffscreenResources(device, createInfo);
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
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
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

void Renderer::allocateSwapchainResourcesMemory() {
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
    // Create the descriptor pool.
    VkDescriptorPoolSize descriptorPoolSizes[] = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, framesInFlight }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .maxSets       = framesInFlight,
        .poolSizeCount = ARRAY_SIZE(descriptorPoolSizes),
        .pPoolSizes    = descriptorPoolSizes
    };

    vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);

    // Allocate the descriptor sets.
    descriptorSets = new VkDescriptorSet[framesInFlight];

    VkDescriptorSetLayout* descriptorSetLayouts = new VkDescriptorSetLayout[framesInFlight];

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        descriptorSetLayouts[i] = descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = nullptr,
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = framesInFlight,
        .pSetLayouts        = descriptorSetLayouts
    };

    vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSets);

    delete[] descriptorSetLayouts;

    // Allocate the command buffers.
    normalCommandBuffers = new VkCommandBuffer[framesInFlight];
    transientCommandBuffers = new VkCommandBuffer[framesInFlight];

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = normalCommandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = framesInFlight
    };

    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, normalCommandBuffers);

    commandBufferAllocateInfo.commandPool = transientCommandPool;

    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, transientCommandBuffers);

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

void Renderer::allocateOffscreenResourcesMemory() {
    offscreenImages = new VkImage[framesInFlight];
    offscreenImageViews = new VkImageView[framesInFlight];
}

void Renderer::createOffscreenResources(Device& device, const RendererCreateInfo& createInfo) {
    // Create the off-screen images.
    VkExtent2D extent = createInfo.surfaceCapabilities->currentExtent;

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        VkImageCreateInfo imageCreateInfo = {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = 0,
            .imageType             = VK_IMAGE_TYPE_2D,
            .format                = VK_FORMAT_A2B10G10R10_UNORM_PACK32,
            .extent                = { extent.width, extent.height, 1 },
            .mipLevels             = 1,
            .arrayLayers           = 1,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
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
            .format           = VK_FORMAT_A2B10G10R10_UNORM_PACK32,
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

void Renderer::freeSwapchainResourcesMemory() {
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

    vkFreeCommandBuffers(device, transientCommandPool, framesInFlight, transientCommandBuffers);
    vkFreeCommandBuffers(device, normalCommandPool, framesInFlight, normalCommandBuffers);

    delete[] transientCommandBuffers;
    delete[] normalCommandBuffers;
    delete[] descriptorSets;

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void Renderer::freeOffscreenResourcesMemory() {
    delete[] offscreenImageViews;
    delete[] offscreenImages;
}

void Renderer::destroyOffscreenResources(VkDevice device) {
    for (uint32_t i = 0; i < framesInFlight; ++i) {
        vkDestroyImageView(device, offscreenImageViews[i], nullptr);
    }

    vkFreeMemory(device, offscreenImagesMemory, nullptr);

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        vkDestroyImage(device, offscreenImages[i], nullptr);
    }
}
