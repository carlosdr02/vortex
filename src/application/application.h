#pragma once

#include <graphics.h>
#include "project.h"

class Application {
public:
    Project project;

    Application();
    ~Application();

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
    VkPipelineLayout pipelineLayout;
    VkPipeline rayTracingPipeline;
    ShaderBindingTable shaderBindingTable;

    void createWindow();
    void createEngineResources();
    void createGuiResources();
    void forLackOfABetterName();

    RendererCreateInfo getRendererCreateInfo();
};
