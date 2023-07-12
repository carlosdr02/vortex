#include "game.h"

Game::Game() {
    glfwInit();

    createWindow();
    createEngineResources();
}

Game::~Game() {
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
    surfaceCapabilities = device.getSurfaceCapabilities(surface, window);
    surfaceFormat = device.getSurfaceFormat(surface);
    renderPass = createRenderPass(device.logical, VK_FORMAT_R16G16B16A16_SFLOAT);

    RendererCreateInfo rendererCreateInfo = {
        .surface             = surface,
        .surfaceCapabilities = &surfaceCapabilities,
        .surfaceFormat       = surfaceFormat,
        .framesInFlight      = 2,
        .format              = VK_FORMAT_R16G16B16A16_SFLOAT,
        .renderPass          = renderPass
    };

    renderer = Renderer(device, rendererCreateInfo);
}
