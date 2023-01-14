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

void Interface::update(float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_W)) camera.translation += camera.getForwardVector() * camera.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D)) camera.translation -= camera.getRightVector() * camera.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S)) camera.translation -= camera.getForwardVector() * camera.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D)) camera.translation += camera.getRightVector() * camera.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_SPACE)) camera.translation += glm::vec3(0.0f, 1.0f, 0.0f) * camera.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) camera.translation -= glm::vec3(0.0f, 1.0f, 0.0f) * camera.speed * deltaTime;
}
