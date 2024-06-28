#include "gui.h"

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

using namespace ImGui;

static void renderMainMenuBar() {
    if (BeginMainMenuBar()) {
        if (BeginMenu("File")) {
            EndMenu();
        }

        EndMainMenuBar();
    }
}

void renderGui() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    NewFrame();

    renderMainMenuBar();

    Render();
}
