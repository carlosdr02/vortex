#pragma once

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

class ImGuiLayer {
public:
    ImGuiLayer();
    void destroy();

    ImDrawData* render();

private:
    bool projectPanel = true;
    bool hierarchyPanel = true;
    bool propertiesPanel = true;

    void pollEvents();
    void renderMainMenuBar();
    void renderViewport();
    void renderProjectPanel();
    void renderHierarchyPanel();
    void renderPropertiesPanel();
};
