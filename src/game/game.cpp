#include "game.h"

Game::Game() {
    glfwInit();

    createWindow();
    createEngineResources();
}

Game::~Game() {
    renderer.waitIdle(device.logical);
    renderer.destroy(device.logical);
    device.destroy();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void Game::run() {
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        camera.update(window, deltaTime);

        glm::mat4 cameraMatrices[] = {
            camera.getInverseViewMatrix(),
            camera.getInverseProjectionMatrix(static_cast<float>(extent.width) / extent.height)
        };

        if (!renderer.render(device, cameraMatrices, extent)) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            while (width == 0 || height == 0) {
                glfwWaitEvents();
                glfwGetFramebufferSize(window, &width, &height);
            }

            RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();

            renderer.waitIdle(device.logical);
            renderer.recreate(device, rendererCreateInfo);
            renderer.recordCommandBuffers(device.logical);
        }
    }
}

void Game::createWindow() {
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1600, 900, "Achantcraft", nullptr, nullptr);

    glfwSetWindowUserPointer(window, &camera);

    glfwSetCursorPosCallback(window, cursorPosCallback);
}

void Game::createEngineResources() {
    instance = createInstance();
    glfwCreateWindowSurface(instance, window, nullptr, &surface);
    device = Device(instance, surface);
    surfaceFormat = device.getSurfaceFormat(surface);

    RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();

    renderer = Renderer(device, rendererCreateInfo);
    renderer.recordCommandBuffers(device.logical);
}

RendererCreateInfo Game::getRendererCreateInfo() {
    surfaceCapabilities = device.getSurfaceCapabilities(surface, window);
    extent = surfaceCapabilities.currentExtent;

    RendererCreateInfo rendererCreateInfo = {
        .surface             = surface,
        .surfaceCapabilities = &surfaceCapabilities,
        .surfaceFormat       = surfaceFormat,
        .framesInFlight      = 2,
        .uniformDataSize     = 2 * sizeof(glm::mat4)
    };

    return rendererCreateInfo;
}
