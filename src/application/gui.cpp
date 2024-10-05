#include "gui.h"

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "file_dialog.h"

using namespace ImGui;

static bool settingsWindow = false;
static bool contentBrowserWindow = true;

static void renderMainMenuBar() {
    if (BeginMainMenuBar()) {
        if (BeginMenu("File")) {
            EndMenu();
        }

        if (BeginMenu("Edit")) {
            MenuItem("Settings", NULL, &settingsWindow);

            EndMenu();
        }

        if (BeginMenu("View")) {
            EndMenu();
        }

        if (BeginMenu("Tools")) {
            EndMenu();
        }

        if (BeginMenu("Window")) {
            MenuItem("Content browser", NULL, &contentBrowserWindow);

            EndMenu();
        }

        if (BeginMenu("Help")) {
            EndMenu();
        }

        EndMainMenuBar();
    }
}

static void renderSettingsWindow() {
    Begin("Settings", &settingsWindow);

    if (BeginTabBar("settings_window_tab_bar")) {
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

static void renderContentBrowserWindow(GLFWwindow* window, Project& project) {
    Begin("Content browser", &contentBrowserWindow);

    if (BeginPopupContextWindow()) {
        if (MenuItem("Import...")) {
            std::string file = openFileDialog(window);
            project.import(file);
        }

        EndPopup();
    }

    End();
}

void renderGui(GLFWwindow* window, Project& project) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    NewFrame();

    renderMainMenuBar();
    if (settingsWindow) renderSettingsWindow();
    if (contentBrowserWindow) renderContentBrowserWindow(window, project);

    Render();
}
