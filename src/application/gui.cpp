#include "gui.h"

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

using namespace ImGui;

static bool settingsWindow = false;
static bool contentBrowserWindow = true;
static bool openOrCreateProjectModal = true;
static bool createNewProjectModal = false;

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

static void renderContentBrowserWindow() {
    Begin("Content browser", &contentBrowserWindow);
    End();
}

static void renderOpenOrCreateProjectModal() {
    OpenPopup("Open or create a project");

    ImGuiIO& io = ImGui::GetIO();
    SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (BeginPopupModal("Open or create a project", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
        if (Button("Create a new project")) {
            createNewProjectModal = true;
            CloseCurrentPopup();
            openOrCreateProjectModal = false;
        }

        EndPopup();
    }
}

static void renderCreateNewProjectModal() {
    OpenPopup("Create new project");

    ImGuiIO& io = ImGui::GetIO();
    SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (BeginPopupModal("Create new project", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
        static char name[128] = "";
        static char location[256] = "";

        if (Button("<")) {
            openOrCreateProjectModal = true;
            CloseCurrentPopup();
            createNewProjectModal = false;
            strcpy(name, "");
            strcpy(location, "");
        }

        InputText("Name", name, sizeof(name));
        InputText("Location", location, sizeof(location));

        SameLine();

        if (Button("...")) {

        }

        EndPopup();
    }
}

void renderGui() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    NewFrame();

    renderMainMenuBar();
    if (settingsWindow) renderSettingsWindow();
    if (contentBrowserWindow) renderContentBrowserWindow();
    if (openOrCreateProjectModal) renderOpenOrCreateProjectModal();
    if (createNewProjectModal) renderCreateNewProjectModal();

    Render();
}
