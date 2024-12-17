#include "gui.h"

#include <stack>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

using namespace ImGui;

static bool settingsWindow = false;
static bool projectPanel = true;
static bool openOrCreateProjectModal = true;
static bool createNewProjectModal = false;
static std::filesystem::path selectedPath;
static std::stack<std::filesystem::path> lastVisitedPaths;
static std::stack<std::filesystem::path> cosa;

static void selectPath(const std::filesystem::path& path) {
    if (selectedPath != path) {
        lastVisitedPaths.push(selectedPath);
        selectedPath = path;
        cosa = std::stack<std::filesystem::path>();
    }
}

static void renderMainMenuBar() {
    if (BeginMainMenuBar()) {
        if (BeginMenu("File")) {
            EndMenu();
        }

        if (BeginMenu("Edit")) {
            MenuItem("Settings", nullptr, &settingsWindow);

            EndMenu();
        }

        if (BeginMenu("View")) {
            EndMenu();
        }

        if (BeginMenu("Tools")) {
            EndMenu();
        }

        if (BeginMenu("Window")) {
            MenuItem("Project panel", nullptr, &projectPanel);

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

static bool hasSubdirectories(const std::filesystem::path& path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
            return true;
        }
    }

    return false;
}

static void renderProjectTree(const std::filesystem::path& path) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGuiTreeNodeFlags nodeFlags = flags | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    ImGuiTreeNodeFlags leafFlags = flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (path == selectedPath) {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    bool nodeClicked = TreeNodeEx(path.filename().c_str(), nodeFlags);
    if (IsItemClicked() && !IsItemToggledOpen()) {
        selectPath(path);
    }

    if (nodeClicked) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_directory()) {
                if (hasSubdirectories(entry.path())) {
                    renderProjectTree(entry.path());
                } else {
                    if (entry.path() == selectedPath) {
                        leafFlags |= ImGuiTreeNodeFlags_Selected;
                    }

                    TreeNodeEx(entry.path().filename().c_str(), leafFlags);
                    if (IsItemClicked() && !IsItemToggledOpen()) {
                        selectPath(entry.path());
                    }
                }
            }
        }

        TreePop();
    }
}

static void renderProjectFiles() {
    static std::filesystem::path selectedFile;

    float itemWidth = 75.0f;
    float spacing = GetStyle().ItemSpacing.x;
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float currentX = 0.0f;

    PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

    for (const auto& entry : std::filesystem::directory_iterator(selectedPath)) {
        if (currentX + itemWidth > availableWidth) {
            NewLine();
            currentX = 0.0f;
        }

        if (entry.path() != selectedFile) {
            PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        }

        if (Button(entry.path().filename().c_str(), ImVec2(itemWidth, itemWidth))) {
            selectedFile = entry.path();
        }

        PopStyleColor();

        if (IsItemHovered() && IsMouseDoubleClicked(0) && entry.is_directory()) {
            selectPath(entry.path());
        }

        currentX += itemWidth + spacing;
        SameLine();
    }

    PopStyleVar();
}

static void renderProjectPanel(Project& project) {
    Begin("Project", &projectPanel);

    if (selectedPath != "") {
        BeginDisabled(lastVisitedPaths.empty());
        if (Button("<")) {
            cosa.push(selectedPath);
            selectedPath = lastVisitedPaths.top();
            lastVisitedPaths.pop();
        }
        EndDisabled();

        SameLine();

        BeginDisabled(cosa.empty());
        if (Button(">")) {
            lastVisitedPaths.push(selectedPath);
            selectedPath = cosa.top();
            cosa.pop();
        }
        EndDisabled();

        SameLine();

        bool buttonDisabled = selectedPath == project.getAssetsDirectoryPath();
        BeginDisabled(buttonDisabled);
        if (Button("^")) {
            selectPath(selectedPath.parent_path());
        }
        EndDisabled();

        SameLine();

        if (Button("H")) {
            selectPath(project.getAssetsDirectoryPath());
        }

        if (BeginTable("project_panel_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV, GetContentRegionAvail())) {
            TableNextColumn();
            renderProjectTree(project.getAssetsDirectoryPath());

            TableNextColumn();
            renderProjectFiles();

            EndTable();
        }
    }

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
        InputText("Name", name, sizeof(name));

        static char location[256] = "";
        InputText("Location", location, sizeof(location));

        SameLine();

        if (Button("...")) {

        }

        PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        Text(""); // TODO: Error: Invalid path.
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
            selectedPath = app.project.getAssetsDirectoryPath();
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
    if (projectPanel) renderProjectPanel(app.project);
    if (openOrCreateProjectModal) renderOpenOrCreateProjectModal();
    if (createNewProjectModal) renderCreateNewProjectModal(app);

    Render();
}
