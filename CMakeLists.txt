CMAKE_MINIMUM_REQUIRED(VERSION 3.21)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

PROJECT(Vortex VERSION 1.0.0)

# Engine
ADD_LIBRARY(engine
    src/engine/graphics.cpp
)

TARGET_INCLUDE_DIRECTORIES(engine PUBLIC src/engine)

# Application
ADD_LIBRARY(application
    src/application/application.cpp
)

TARGET_INCLUDE_DIRECTORIES(application PUBLIC src/application)

# Interface
ADD_LIBRARY(interface
    src/interface/gui.cpp
)

TARGET_INCLUDE_DIRECTORIES(interface PUBLIC src/interface)

# Vulkan
FIND_PACKAGE(Vulkan REQUIRED)

TARGET_INCLUDE_DIRECTORIES(engine PUBLIC ${Vulkan_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(engine ${Vulkan_LIBRARIES})

# GLFW
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

ADD_SUBDIRECTORY(dependencies/glfw)

TARGET_LINK_LIBRARIES(engine glfw)
TARGET_LINK_LIBRARIES(interface glfw)

# ImGui
ADD_LIBRARY(imgui
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_demo.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_tables.cpp
    dependencies/imgui/imgui_widgets.cpp
    dependencies/imgui/backends/imgui_impl_vulkan.cpp
    dependencies/imgui/backends/imgui_impl_glfw.cpp
)

TARGET_LINK_LIBRARIES(imgui glfw)

TARGET_INCLUDE_DIRECTORIES(imgui PUBLIC ${Vulkan_INCLUDE_DIRS})
TARGET_INCLUDE_DIRECTORIES(imgui PUBLIC
    dependencies/imgui
    dependencies/imgui/backends
)

TARGET_LINK_LIBRARIES(engine imgui)
TARGET_LINK_LIBRARIES(interface imgui)

# Executable
ADD_EXECUTABLE(vortex src/main.cpp)

TARGET_LINK_LIBRARIES(application engine)
TARGET_LINK_LIBRARIES(application interface)

TARGET_LINK_LIBRARIES(vortex application)
