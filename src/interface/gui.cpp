#include "gui.h"

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

using namespace ImGui;

ImGuiLayer::ImGuiLayer() {
    IMGUI_CHECKVERSION();
    CreateContext();

    ImGuiIO& io = GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void ImGuiLayer::destroy() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    DestroyContext();
}

ImDrawData* ImGuiLayer::render(VkDescriptorSet descriptorSet) {
    pollEvents();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    NewFrame();

    DockSpaceOverViewport();

    renderMainMenuBar();
    renderViewport(descriptorSet);
    if (projectPanel) renderProjectPanel();
    if (hierarchyPanel) renderHierarchyPanel();
    if (propertiesPanel) renderPropertiesPanel();

    Render();

    auto drawData = GetDrawData();

    return drawData;
}

void ImGuiLayer::pollEvents() {
    if (IsKeyPressed(ImGuiKey_Space)) projectPanel = !projectPanel;
}

void ImGuiLayer::renderMainMenuBar() {
    if (BeginMainMenuBar()) {
        if (BeginMenu("File")) {
            MenuItem("New...", "Ctrl+N");
            MenuItem("Open...", "Ctrl+O");

            if (BeginMenu("Recent")) {

                EndMenu();
            }

            Separator();
            MenuItem("Save", "Ctrl+S");
            MenuItem("Save as...", "Ctrl+Shift+S");
            Separator();
            MenuItem("Close", "Ctrl+W");
            Separator();
            MenuItem("Exit", "Ctrl+Q"); // TODO:

            EndMenu();
        }

        if (BeginMenu("View")) {
            MenuItem("Project panel", "Space", &projectPanel);
            MenuItem("Hierarchy panel", nullptr, &hierarchyPanel);
            MenuItem("Properties panel", nullptr, &propertiesPanel);

            EndMenu();
        }

        EndMainMenuBar();
    }
}

void ImGuiLayer::renderViewport(VkDescriptorSet descriptorSet) {
    SetNextWindowBgAlpha(1.0f);
    Begin("Viewport");
    Image((ImTextureID)descriptorSet, GetContentRegionAvail());
    End();
}

void ImGuiLayer::renderProjectPanel() {
    SetNextWindowBgAlpha(1.0f);
    Begin("Project", &projectPanel);
    End();
}

void ImGuiLayer::renderHierarchyPanel() {
    SetNextWindowBgAlpha(1.0f);
    Begin("Hierarchy", &hierarchyPanel);
    End();
}

void ImGuiLayer::renderPropertiesPanel() {
    SetNextWindowBgAlpha(1.0f);
    Begin("Properties", &propertiesPanel);
    End();
}
