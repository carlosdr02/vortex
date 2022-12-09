#ifndef GUI_H
#define GUI_H

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

void initGui();
ImDrawData* renderGui();
void terminateGui();

#endif // !GUI_H
