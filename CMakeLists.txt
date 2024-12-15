cmake_minimum_required(VERSION 3.7)

add_compile_definitions(SOURCE_DIR_PATH="${CMAKE_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Vortex VERSION 1.0.0)

# Vulkan
find_package(Vulkan REQUIRED)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(dependencies/glfw)

# Dear ImGui
add_library(imgui
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_demo.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_tables.cpp
    dependencies/imgui/imgui_widgets.cpp
    dependencies/imgui/backends/imgui_impl_vulkan.cpp
    dependencies/imgui/backends/imgui_impl_glfw.cpp
)

target_include_directories(imgui PUBLIC dependencies/imgui)
target_include_directories(imgui PUBLIC dependencies/imgui/backends)

target_include_directories(imgui PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(imgui ${Vulkan_LIBRARIES})

target_link_libraries(imgui glfw)

# Engine
add_library(engine
    src/engine/graphics.cpp
)

target_include_directories(engine PUBLIC src/engine)

target_link_libraries(engine imgui)

# Application
add_library(application
    src/application/project.cpp
    src/application/application.cpp
    src/application/gui.cpp
)

target_include_directories(application PUBLIC src/application)

target_link_libraries(application engine)

# Executable
add_executable(vortex src/main.cpp)

target_link_libraries(vortex application)
