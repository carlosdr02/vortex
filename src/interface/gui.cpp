#include "gui.h"

using namespace ImGui;

static bool projectPanel = true;
static bool hierarchyPanel = true;
static bool propertiesPanel = true;

void initGui() {
    IMGUI_CHECKVERSION();
    CreateContext();

    ImGuiIO& io = GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
}

static void processInput() {
    if (IsKeyPressed(ImGuiKey_Space)) projectPanel = !projectPanel;
}

static void renderMenuBar() {
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

static void renderViewport() {
    SetNextWindowBgAlpha(1.0f);
    Begin("Viewport");
    End();
}

static void renderProjectPanel() {
    SetNextWindowBgAlpha(1.0f);
    Begin("Project", &projectPanel);
    End();
}

static void renderHierarchyPanel() {
    SetNextWindowBgAlpha(1.0f);
    Begin("Hierarchy", &hierarchyPanel);
    End();
}

static void renderPropertiesPanel() {
    SetNextWindowBgAlpha(1.0f);
    Begin("Properties", &propertiesPanel);
    End();
}

ImDrawData* renderGui() {
    processInput();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    NewFrame();

    DockSpaceOverViewport();

    renderMenuBar();
    renderViewport();
    if (projectPanel) renderProjectPanel();
    if (hierarchyPanel) renderHierarchyPanel();
    if (propertiesPanel) renderPropertiesPanel();

    Render();

    UpdatePlatformWindows();
    RenderPlatformWindowsDefault();

    ImDrawData* drawData = GetDrawData();

    return drawData;
}

void terminateGui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    DestroyContext();
}
