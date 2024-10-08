CMAKE_MINIMUM_REQUIRED(VERSION 3.7)

ADD_COMPILE_DEFINITIONS(SOURCE_DIR_PATH="${CMAKE_SOURCE_DIR}")

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

PROJECT(Vortex VERSION 1.0.0)

# Vulkan
FIND_PACKAGE(Vulkan REQUIRED)

# GLFW
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

ADD_SUBDIRECTORY(dependencies/glfw)

# Dear ImGui
ADD_LIBRARY(imgui
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_demo.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_tables.cpp
    dependencies/imgui/imgui_widgets.cpp
    dependencies/imgui/backends/imgui_impl_vulkan.cpp
    dependencies/imgui/backends/imgui_impl_glfw.cpp
)

TARGET_INCLUDE_DIRECTORIES(imgui PUBLIC dependencies/imgui)
TARGET_INCLUDE_DIRECTORIES(imgui PUBLIC dependencies/imgui/backends)

TARGET_INCLUDE_DIRECTORIES(imgui PUBLIC ${Vulkan_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(imgui ${Vulkan_LIBRARIES})

TARGET_LINK_LIBRARIES(imgui glfw)

# Engine
ADD_LIBRARY(engine
    src/engine/graphics.cpp
)

TARGET_INCLUDE_DIRECTORIES(engine PUBLIC src/engine)

TARGET_LINK_LIBRARIES(engine imgui)

# Application
ADD_LIBRARY(application
    src/application/platform/windows/file_dialog.cpp
    src/application/application.cpp
    src/application/gui.cpp
)

TARGET_INCLUDE_DIRECTORIES(application PUBLIC src/application)

TARGET_LINK_LIBRARIES(application engine)

# Executable
ADD_EXECUTABLE(vortex src/main.cpp)

TARGET_LINK_LIBRARIES(vortex application)
