#pragma once

#include <graphics.h>
#include <camera.h>

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    GLFWwindow* window;
    Camera camera;
    VkInstance instance;
    VkSurfaceKHR surface;
    Device device;
    VkSurfaceFormatKHR surfaceFormat;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkExtent2D extent;
    Renderer renderer;

    void createWindow();
    void createEngineResources();

    RendererCreateInfo getRendererCreateInfo();
};
