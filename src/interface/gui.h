#pragma once

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

class ImGuiLayer {
public:
    ImGuiLayer();
    void destroy();

    void render(VkDescriptorSet descriptorSet);

private:
    bool projectPanel = true;
    bool hierarchyPanel = true;
    bool propertiesPanel = true;

    void pollEvents();
    void renderMainMenuBar();
    void renderViewport(VkDescriptorSet descriptorSet);
    void renderProjectPanel();
    void renderHierarchyPanel();
    void renderPropertiesPanel();
};
