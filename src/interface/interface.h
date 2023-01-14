#pragma once

#include <GLFW/glfw3.h>

#include "camera.h"

class Interface {
public:
    Camera camera;

    Interface() = default;
    Interface(GLFWwindow* window);

private:
    GLFWwindow* window;
};
