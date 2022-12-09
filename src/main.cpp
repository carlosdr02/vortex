#include <graphics.h>
#include <gui.h>
#include <camera.h>

int main() {
    glfwInit();

    const char* applicationName = "Achantcraft";

    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1600, 900, applicationName, nullptr, nullptr);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkInstance instance = createInstance(applicationName, VK_MAKE_VERSION(1, 0, 0));

    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, window, nullptr, &surface);

    Device device(instance, surface);

    VkSurfaceCapabilitiesKHR surfaceCapabilities = device.getSurfaceCapabilities(surface, window);
    VkSurfaceFormatKHR surfaceFormat = device.getSurfaceFormat(surface);

    VkRenderPass renderPass = createRenderPass(device.logical, surfaceFormat.format);

    VkDescriptorPool guiDescriptorPool = createGuiDescriptorPool(device.logical);

    initGui();

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

    RendererCreateInfo rendererCreateInfo = {
        .surface             = surface,
        .surfaceCapabilities = &surfaceCapabilities,
        .surfaceFormat       = surfaceFormat,
        .renderPass          = renderPass,
        .cameraDataSize      = 2 * sizeof(glm::mat4),
        .framesInFlight      = 6
    };

    Renderer renderer(device, rendererCreateInfo);

    Camera camera = {
        .translation = glm::vec3(0.0f, 0.0f, 1.0f),
        .orientation = glm::vec3(0.0f, 0.0f, -1.0f),
        .speed       = 1.0f,
        .sensitivity = 0.1f,
        .fov         = 90.0f,
        .aspectRatio = width / (float)height,
        .nearPlane   = 0.1f,
        .farPlane    = 100.0f
    };

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        camera.update(window, deltaTime);

        glm::mat4 cameraMatrices[] = {
            camera.getInverseViewMatrix(),
            camera.getInverseProjectionMatrix()
        };

        ImDrawData* drawData = renderGui();

        if (!renderer.draw(device, cameraMatrices, surfaceCapabilities.currentExtent, renderPass, drawData)) {
            do {
                glfwWaitEvents();
                glfwGetFramebufferSize(window, &width, &height);
            } while(width == 0 || height == 0);

            surfaceCapabilities = device.getSurfaceCapabilities(surface, window);

            renderer.waitIdle(device.logical);
            renderer.recreate(device, rendererCreateInfo);

            camera.aspectRatio = width / (float)height;
        }
    }

    renderer.waitIdle(device.logical);
    renderer.destroy(device.logical);

    terminateGui();

    vkDestroyDescriptorPool(device.logical, guiDescriptorPool, nullptr);
    vkDestroyRenderPass(device.logical, renderPass, nullptr);

    device.destroy();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
