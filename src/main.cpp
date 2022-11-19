#include <graphics.h>
#include <camera.h>

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    const char* applicationName = "Achantcraft";
    VkInstance instance = createInstance(applicationName, VK_MAKE_VERSION(1, 0, 0));

    int width = 1600, height = 900;
    Window window(instance, width, height, applicationName);

    Device device(instance, window.surface);

    VkSurfaceCapabilitiesKHR surfaceCapabilities = device.getSurfaceCapabilities(window);
    VkSurfaceFormatKHR surfaceFormat = device.getSurfaceFormat(window.surface);
    VkPresentModeKHR presentMode = device.getSurfacePresentMode(window.surface);
    VkFormat depthFormat = device.getDepthFormat();

    VkRenderPass renderPass = createRenderPass(device.logical, surfaceFormat.format, depthFormat);

    VkQueue graphicsQueue = device.getQueue(0);
    VkQueue presentQueue = device.getQueue(1);

    RendererCreateInfo rendererCreateInfo = {
        .surface             = window.surface,
        .surfaceCapabilities = &surfaceCapabilities,
        .surfaceFormat       = surfaceFormat,
        .presentMode         = presentMode,
        .depthFormat         = depthFormat,
        .renderPass          = renderPass,
        .cameraDataSize      = sizeof(glm::mat4),
        .framesInFlight      = 3,
        .graphicsQueue       = graphicsQueue,
        .presentQueue        = presentQueue
    };

    Renderer renderer(device, rendererCreateInfo);
    renderer.recordCommandBuffers(device.logical, renderPass, surfaceCapabilities.currentExtent);

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

    glfwSetWindowUserPointer(window, &camera);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        camera.update(window, deltaTime);
        glm::mat4 viewProjection = camera.getViewProjectionMatrix();

        if (!renderer.draw(device.logical, &viewProjection)) {
            do {
                glfwWaitEvents();
                glfwGetFramebufferSize(window, &width, &height);
            } while(width == 0 || height == 0);

            surfaceCapabilities = device.getSurfaceCapabilities(window);

            renderer.waitIdle(device.logical);
            renderer.recreate(device, rendererCreateInfo);
            renderer.recordCommandBuffers(device.logical, renderPass, surfaceCapabilities.currentExtent);

            camera.aspectRatio = width / (float)height;
        }
    }

    renderer.waitIdle(device.logical);
    renderer.destroy(device.logical);

    vkDestroyRenderPass(device.logical, renderPass, nullptr);

    device.destroy();
    window.destroy(instance);

    vkDestroyInstance(instance, nullptr);

    glfwTerminate();
}
