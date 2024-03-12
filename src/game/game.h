#pragma once

#include <graphics.h>

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
    VkSurfaceFormatKHR surfaceFormat;
    VkRenderPass renderPass;
    VkDescriptorPool guiDescriptorPool;
    Renderer renderer;

    void createWindow();
    void createEngineResources();

    RendererCreateInfo getRendererCreateInfo();
};
