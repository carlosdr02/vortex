#pragma once

#include <GLFW/glfw3.h>

#include "camera.h"

class Interface {
public:
    Camera camera;

    Interface() = default;
    Interface(GLFWwindow* window);

    void update(float deltaTime);

private:
    GLFWwindow* window;
};
