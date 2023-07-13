#include "game.h"

Game::Game() {
    glfwInit();

    createWindow();
    createEngineResources();
}

Game::~Game() {
    renderer.waitIdle(device.logical);
    renderer.destroy(device.logical);

    vkDestroyRenderPass(device.logical, renderPass, nullptr);

    device.destroy();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void Game::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (!renderer.render(device, renderPass, surfaceCapabilities.currentExtent)) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            while (width == 0 || height == 0) {
                glfwWaitEvents();
                glfwGetFramebufferSize(window, &width, &height);
            }

            renderer.waitIdle(device.logical);

            RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();
            renderer.resize(device.logical, rendererCreateInfo);
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
    surfaceFormat = device.getSurfaceFormat(surface);
    renderPass = createRenderPass(device.logical, surfaceFormat.format);

    RendererCreateInfo rendererCreateInfo = getRendererCreateInfo();
    renderer = Renderer(device, rendererCreateInfo);
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
