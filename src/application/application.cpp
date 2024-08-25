#include "application.h"

#include <thread>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "gui.h"

Application::Application() {
    glfwInit();

    createWindow();
    createEngineResources();
    createGuiResources();
    forLackOfABetterName();
}

Application::~Application() {
    renderer.waitIdle(device.logical);
    renderer.destroy(device.logical);
    shaderBindingTable.destroy(device.logical);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyPipeline(device.logical, rayTracingPipeline, nullptr);
    vkDestroyPipelineLayout(device.logical, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device.logical, guiDescriptorPool, nullptr);
    vkDestroyRenderPass(device.logical, renderPass, nullptr);

    device.destroy();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::run() {
    VkExtent2D extent = surfaceCapabilities.currentExtent;
    renderer.recordCommandBuffers(device.logical, pipelineLayout, rayTracingPipeline, shaderBindingTable, extent);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        renderGui();

        if (!renderer.render(device, renderPass, extent)) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            while (width == 0 || height == 0) {
                glfwWaitEvents();
                glfwGetFramebufferSize(window, &width, &height);
            }

            renderer.waitIdle(device.logical);

            RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();
            renderer.resize(device, rendererCreateInfo);
            extent = surfaceCapabilities.currentExtent;
            renderer.recordCommandBuffers(device.logical, pipelineLayout, rayTracingPipeline, shaderBindingTable, extent);
        }
    }
}

void Application::createWindow() {
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1600, 900, "Vortex", nullptr, nullptr);
}

void Application::createEngineResources() {
    instance = createInstance();
    glfwCreateWindowSurface(instance, window, nullptr, &surface);
    device = Device(instance, surface);
    loadFunctionPointers(device.logical);
    surfaceFormat = device.getSurfaceFormat(surface);
    renderPass = createRenderPass(device.logical, surfaceFormat.format, false);
    guiDescriptorPool = createGuiDescriptorPool(device.logical);

    RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();
    renderer = Renderer(device, rendererCreateInfo);

    pipelineLayout = createPipelineLayout(device.logical, 1, &renderer.descriptorSetLayout);

    ShaderBindingTableEntry sbtEntries[] = {
        { .stage = SHADER_BINDING_TABLE_STAGE_RAYGEN, .generalShader = "raygen.spv" }
    };

    rayTracingPipeline = createRayTracingPipeline(device.logical, 1, sbtEntries, pipelineLayout);
    shaderBindingTable = ShaderBindingTable(device, 1, sbtEntries);
}

void Application::createGuiResources() {
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo initInfo = {
        .Instance            = instance,
        .PhysicalDevice      = device.physical,
        .Device              = device.logical,
        .QueueFamily         = device.renderQueue.familyIndex,
        .Queue               = device.renderQueue,
        .DescriptorPool      = guiDescriptorPool,
        .RenderPass          = renderPass,
        .MinImageCount       = surfaceCapabilities.minImageCount,
        .ImageCount          = surfaceCapabilities.minImageCount,
        .MSAASamples         = VK_SAMPLE_COUNT_1_BIT,
        .PipelineCache       = VK_NULL_HANDLE,
        .Subpass             = 0,
        .UseDynamicRendering = false,
        .Allocator           = nullptr,
        .CheckVkResultFn     = nullptr,
        .MinAllocationSize   = 0
    };

    ImGui_ImplVulkan_Init(&initInfo);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("../res/fonts/Ubuntu-Regular.ttf", 13.0f);
}

void Application::forLackOfABetterName() {
    const VkDeviceSize sbtSize = shaderBindingTable.size;

    Buffer stagingBuffer(device, sbtSize,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    {
        void* data;
        vkMapMemory(device.logical, stagingBuffer.memory, 0, sbtSize, 0, &data);
        vkGetRayTracingShaderGroupHandles(device.logical, rayTracingPipeline, 0, 1, sbtSize, data);
        vkUnmapMemory(device.logical, stagingBuffer.memory);
    }

    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = device.transferQueue.familyIndex
    };

    VkCommandPool transferCommandPool;
    vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &transferCommandPool);

    commandPoolCreateInfo.queueFamilyIndex = device.renderQueue.familyIndex;

    VkCommandPool renderCommandPool;
    vkCreateCommandPool(device.logical, &commandPoolCreateInfo, nullptr, &renderCommandPool);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = transferCommandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer transferCommandBuffer;
    vkAllocateCommandBuffers(device.logical, &commandBufferAllocateInfo, &transferCommandBuffer);

    commandBufferAllocateInfo.commandPool = renderCommandPool;

    VkCommandBuffer renderCommandBuffer;
    vkAllocateCommandBuffers(device.logical, &commandBufferAllocateInfo, &renderCommandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(transferCommandBuffer, &commandBufferBeginInfo);

    VkBufferCopy region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size      = sbtSize
    };

    vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, shaderBindingTable.buffer, 1, &region);

    VkBufferMemoryBarrier2 bufferMemoryBarrier = {
        .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .pNext               = nullptr,
        .srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT,
        .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask        = 0,
        .dstAccessMask       = 0,
        .srcQueueFamilyIndex = device.transferQueue.familyIndex,
        .dstQueueFamilyIndex = device.renderQueue.familyIndex,
        .buffer              = shaderBindingTable.buffer,
        .offset              = 0,
        .size                = sbtSize
    };

    VkDependencyInfo dependencyInfo = {
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext                    = nullptr,
        .dependencyFlags          = 0,
        .memoryBarrierCount       = 0,
        .pMemoryBarriers          = nullptr,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers    = &bufferMemoryBarrier,
        .imageMemoryBarrierCount  = 0,
        .pImageMemoryBarriers     = nullptr
    };

    vkCmdPipelineBarrier2(transferCommandBuffer, &dependencyInfo);

    vkEndCommandBuffer(transferCommandBuffer);

    vkBeginCommandBuffer(renderCommandBuffer, &commandBufferBeginInfo);

    bufferMemoryBarrier.srcStageMask  = 0;
    bufferMemoryBarrier.srcAccessMask = 0;
    bufferMemoryBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
    bufferMemoryBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

    vkCmdPipelineBarrier2(renderCommandBuffer, &dependencyInfo);

    vkEndCommandBuffer(renderCommandBuffer);

    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    VkSemaphore semaphore;
    vkCreateSemaphore(device.logical, &semaphoreCreateInfo, nullptr, &semaphore);

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    VkFence fence;
    vkCreateFence(device.logical, &fenceCreateInfo, nullptr, &fence);

    VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = transferCommandBuffer,
        .deviceMask    = 0
    };

    VkSemaphoreSubmitInfo semaphoreSubmitInfo = {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = semaphore,
        .value       = 0,
        .stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .deviceIndex = 0
    };

    VkSubmitInfo2 submitInfo = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0,
        .waitSemaphoreInfoCount   = 0,
        .pWaitSemaphoreInfos      = nullptr,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &semaphoreSubmitInfo
    };

    vkQueueSubmit2(device.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);

    commandBufferSubmitInfo.commandBuffer = renderCommandBuffer;

    submitInfo.waitSemaphoreInfoCount   = 1;
    submitInfo.pWaitSemaphoreInfos      = &semaphoreSubmitInfo;
    submitInfo.signalSemaphoreInfoCount = 0;
    submitInfo.pSignalSemaphoreInfos    = nullptr;

    vkQueueSubmit2(device.renderQueue, 1, &submitInfo, fence);

    std::thread([=, this]() mutable {
        vkWaitForFences(device.logical, 1, &fence, VK_TRUE, UINT64_MAX);

        vkDestroyFence(device.logical, fence, nullptr);
        vkDestroySemaphore(device.logical, semaphore, nullptr);
        vkDestroyCommandPool(device.logical, renderCommandPool, nullptr);
        vkDestroyCommandPool(device.logical, transferCommandPool, nullptr);

        stagingBuffer.destroy(device.logical);
    }).detach();
}

RendererCreateInfo Application::getRendererCreateInfo() {
    surfaceCapabilities = device.getSurfaceCapabilities(surface, window);

    RendererCreateInfo rendererCreateInfo = {
        .surface             = surface,
        .surfaceCapabilities = &surfaceCapabilities,
        .surfaceFormat       = surfaceFormat,
        .renderPass          = renderPass,
        .framesInFlight      = 2
    };

    return rendererCreateInfo;
}
