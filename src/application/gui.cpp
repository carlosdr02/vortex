#include "gui.h"

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

using namespace ImGui;

static bool preferencesWindow = false;

static void renderMainMenuBar() {
    if (BeginMainMenuBar()) {
        if (BeginMenu("File")) {
            EndMenu();
        }

        if (BeginMenu("Edit")) {
            if (MenuItem("Preferences")) {
                preferencesWindow = true;
            }

            EndMenu();
        }

        if (BeginMenu("View")) {
            EndMenu();
        }

        if (BeginMenu("Tools")) {
            EndMenu();
        }

        if (BeginMenu("Window")) {
            EndMenu();
        }

        if (BeginMenu("Help")) {
            EndMenu();
        }

        EndMainMenuBar();
    }
}

static void renderPreferencesWindow() {
    if (Begin("Preferences", &preferencesWindow)) {
        if (BeginTabBar("preferences_window_tab_bar")) {
            if (BeginTabItem("General")) {
                EndTabItem();
            }

            if (BeginTabItem("Graphics")) {
                EndTabItem();
            }

            EndTabBar();
        }

        End();
    }
}

void renderGui() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    NewFrame();

    renderMainMenuBar();
    if (preferencesWindow) renderPreferencesWindow();

    Render();
}
