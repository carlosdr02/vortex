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

    ImGuiIO& io = GetIO();
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

static void renderCreateNewProjectModal(Application& app) {
    OpenPopup("Create new project");

    ImGuiIO& io = GetIO();
    SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (BeginPopupModal("Create new project", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
        static char name[128] = "";
        InputText("Name", name, sizeof(name), ImGuiInputTextFlags_CharsNoBlank);

        static char location[256] = "";
        InputText("Location", location, sizeof(location));

        SameLine();

        if (Button("...")) {

        }

        PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        Text("");
        PopStyleColor();

        float buttonWidth = 75.0f;
        float buttonSpacing = GetStyle().ItemSpacing.x;
        float totalWidth = 2 * buttonWidth + buttonSpacing;

        SetCursorPos(ImVec2(GetContentRegionMax().x - totalWidth, GetCursorPosY() + 25.0f));

        if (Button("Cancel", ImVec2(buttonWidth, 0))) {
            openOrCreateProjectModal = true;
            CloseCurrentPopup();
            createNewProjectModal = false;
            strcpy(name, "");
            strcpy(location, "");
        }

        SameLine();

        if (Button("Create", ImVec2(buttonWidth, 0))) {
            std::filesystem::path path = std::filesystem::path(location) / name;
            app.project = Project(path);
            CloseCurrentPopup();
            createNewProjectModal = false;
        }

        EndPopup();
    }
}

void renderGui(Application& app) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    NewFrame();

    renderMainMenuBar();
    if (settingsWindow) renderSettingsWindow();
    if (contentBrowserWindow) renderContentBrowserWindow();
    if (openOrCreateProjectModal) renderOpenOrCreateProjectModal();
    if (createNewProjectModal) renderCreateNewProjectModal(app);

    Render();
}
