#include "interface.h"

static void cursorPosCallback(GLFWwindow* window, double xPos, double yPos) {
    Interface* interface = static_cast<Interface*>(glfwGetWindowUserPointer(window));

    Camera& camera = interface->camera;
    camera.orientate(xPos, yPos);
}

Interface::Interface(GLFWwindow* window) : window(window) {
    glfwSetWindowUserPointer(window, this);

    glfwSetCursorPosCallback(window, cursorPosCallback);
}
