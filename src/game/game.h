#pragma once

#include <graphics.h>
#include <interface.h>

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
    Device device;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkExtent2D extent;
    VkSurfaceFormatKHR surfaceFormat;
    Renderer renderer;
    Interface interface;

    void createWindow();
    void createEngineResources();

    RendererCreateInfo getRendererCreateInfo();
};
