#pragma once

#include <graphics.h>
#include "project.h"
#include "scene.h"

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    GLFWwindow* window;
    Project project;
    Scene scene;
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
