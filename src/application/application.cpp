#include "application.h"

Application::Application() {
    glfwInit();

    createWindow();
    createEngineResources();
    createGuiResources();

    RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();

    renderer = Renderer(device, rendererCreateInfo);
}

Application::~Application() {
    renderer.waitIdle(device.logical);

    renderer.destroy(device.logical);

    vkDestroyDescriptorPool(device.logical, guiDescriptorPool, nullptr);
    vkDestroyRenderPass(device.logical, renderPass, nullptr);

    device.destroy();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void Application::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (!renderer.render(device, renderPass, surfaceCapabilities.currentExtent)) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            while (width == 0 || height == 0) {
                glfwWaitEvents();
                glfwGetFramebufferSize(window, &width, &height);
            }

            surfaceCapabilities = device.getSurfaceCapabilities(surface, window);
            RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();

            renderer.waitIdle(device.logical);
            renderer.recreate(device, rendererCreateInfo);
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
    surfaceCapabilities = device.getSurfaceCapabilities(surface, window);
    surfaceFormat = device.getSurfaceFormat(surface);
    renderPass = createRenderPass(device.logical, surfaceFormat.format);
    guiDescriptorPool = createGuiDescriptorPool(device.logical);
}

void Application::createGuiResources() {
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo initInfo = {
        .Instance        = instance,
        .PhysicalDevice  = device.physical,
        .Device          = device.logical,
        .QueueFamily     = device.renderQueue.familyIndex,
        .Queue           = device.renderQueue,
        .PipelineCache   = VK_NULL_HANDLE,
        .DescriptorPool  = guiDescriptorPool,
        .Subpass         = 0,
        .MinImageCount   = surfaceCapabilities.minImageCount,
        .ImageCount      = surfaceCapabilities.minImageCount,
        .MSAASamples     = VK_SAMPLE_COUNT_1_BIT,
        .Allocator       = nullptr,
        .CheckVkResultFn = nullptr
    };

    ImGui_ImplVulkan_Init(&initInfo, renderPass);

    createGuiFonts(device);
}

RendererCreateInfo Application::getRendererCreateInfo() {
    RendererCreateInfo rendererCreateInfo = {
        .surface             = surface,
        .surfaceCapabilities = &surfaceCapabilities,
        .surfaceFormat       = surfaceFormat,
        .renderPass          = renderPass,
        .framesInFlight      = 6
    };

    return rendererCreateInfo;
}
