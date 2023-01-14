#include "game.h"

Game::Game() {
    glfwInit();

    createWindow();
    createEngineResources();
    interface = Interface(window);
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
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (!renderer.render(device, surfaceCapabilities.currentExtent)) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            while (width == 0 || height == 0) {
                glfwWaitEvents();
                glfwGetFramebufferSize(window, &width, &height);
            }

            RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();

            renderer.waitIdle(device.logical);
            renderer.recreate(device, rendererCreateInfo);
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

    RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();

    renderer = Renderer(device, rendererCreateInfo);
}

RendererCreateInfo Game::getRendererCreateInfo() {
    surfaceCapabilities = device.getSurfaceCapabilities(surface, window);

    RendererCreateInfo rendererCreateInfo = {
        .surface             = surface,
        .surfaceCapabilities = &surfaceCapabilities,
        .surfaceFormat       = device.getSurfaceFormat(surface),
        .framesInFlight      = 3,
        .uniformDataSize     = 2 * sizeof(glm::mat4)
    };

    return rendererCreateInfo;
}
