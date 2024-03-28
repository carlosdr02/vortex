#include "game.h"

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "gui.h"

Game::Game() {
    glfwInit();

    createWindow();
    createEngineResources();
    createGuiResources();
}

Game::~Game() {
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

void Game::run() {
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
            renderer.recordCommandBuffers(device.logical, pipelineLayout, rayTracingPipeline, shaderBindingTable, extent);
        }
    }
}

void Game::createWindow() {
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1600, 900, "Achantcraft", nullptr, nullptr);
}

void Game::createEngineResources() {
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
    shaderBindingTable = ShaderBindingTable(device, 1, sbtEntries, rayTracingPipeline);
}

void Game::createGuiResources() {
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
}

RendererCreateInfo Game::getRendererCreateInfo() {
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
